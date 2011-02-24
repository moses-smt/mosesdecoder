/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#ifdef MPI_ENABLE
#include <boost/mpi.hpp>
namespace mpi = boost::mpi;
#endif

#include "FeatureVector.h"
#include "StaticData.h"
#include "ChartTrellisPathList.h"
#include "ChartTrellisPath.h"
#include "ScoreComponentCollection.h"
#include "Decoder.h"
#include "Optimiser.h"

using namespace Mira;
using namespace std;
using namespace Moses;
namespace po = boost::program_options;

void OutputNBestList(const MosesChart::TrellisPathList &nBestList, const TranslationSystem* system, long translationId);

bool loadSentences(const string& filename, vector<string>& sentences) {
  ifstream in(filename.c_str());
  if (!in) return false;
  string line;
  while(getline(in,line)) {
    sentences.push_back(line);
  }
  return true;
}

struct RandomIndex {
  ptrdiff_t operator() (ptrdiff_t max) {
    return static_cast<ptrdiff_t>(rand() % max);
  }
};

int main(int argc, char** argv) {
  size_t rank = 0; size_t size = 1;
#ifdef MPI_ENABLE
  mpi::environment env(argc,argv);
  mpi::communicator world;
  rank = world.rank();
  size = world.size();
#endif
  cerr << "Rank: " << rank << " Size: " << size << endl;

  bool help;
  int verbosity;
  string mosesConfigFile;
  string inputFile;
  vector<string> referenceFiles;
  size_t epochs;
  string learner;
  bool shuffle;
  bool hildreth;
  size_t mixFrequency;
  size_t weightDumpFrequency;
  string weightDumpStem;
  float marginScaleFactor;
  float marginScaleFactorStep;
  float marginScaleFactorMin;
  bool weightedLossFunction;
  size_t n;
  size_t batchSize;
  bool distinctNbest;
  bool onlyViolatedConstraints;
  bool accumulateWeights;
  float historySmoothing;
  bool useScaledReference;
  bool scaleByInputLength;
  float BPfactor;
  float slack;
  float slack_step;
  float slack_max;
  size_t maxNumberOracles;
  bool accumulateMostViolatedConstraints;
  bool pastAndCurrentConstraints;
  bool suppressConvergence;
  bool ignoreUWeight;
  bool ignoreWeirdUpdates;
  bool logFeatureValues;
  size_t baseOfLog;
  float clipping;
  bool fixedClipping;
  string decoder_settings;
  float min_weight_change;
  bool devBleu;
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
      ("config,f",po::value<string>(&mosesConfigFile),"Moses ini file")
      ("verbosity,v", po::value<int>(&verbosity)->default_value(0), "Verbosity level")
      ("input-file,i",po::value<string>(&inputFile),"Input file containing tokenised source")
      ("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training")
      ("epochs,e", po::value<size_t>(&epochs)->default_value(1), "Number of epochs")
      ("learner,l", po::value<string>(&learner)->default_value("mira"), "Learning algorithm")
      ("mix-frequency", po::value<size_t>(&mixFrequency)->default_value(1), "How often per epoch to mix weights, when using mpi")
      ("weight-dump-stem", po::value<string>(&weightDumpStem)->default_value("weights"), "Stem of filename to use for dumping weights")
      ("weight-dump-frequency", po::value<size_t>(&weightDumpFrequency)->default_value(1), "How often per epoch to dump weights")
      ("shuffle", po::value<bool>(&shuffle)->default_value(false), "Shuffle input sentences before processing")
	    ("hildreth", po::value<bool>(&hildreth)->default_value(true), "Use Hildreth's optimisation algorithm")
	    ("msf", po::value<float>(&marginScaleFactor)->default_value(1.0), "Margin scale factor, regularises the update by scaling the enforced margin")
	    ("msf-step", po::value<float>(&marginScaleFactorStep)->default_value(0), "Decrease margin scale factor iteratively by the value provided")
	    ("msf-min", po::value<float>(&marginScaleFactorMin)->default_value(1.0), "Minimum value that margin is scaled by")
	    ("weighted-loss-function", po::value<bool>(&weightedLossFunction)->default_value(false), "Weight the loss of a hypothesis by its Bleu score")
	    ("nbest,n", po::value<size_t>(&n)->default_value(10), "Number of translations in nbest list")
	    ("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
	    ("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(false), "Use nbest list with distinct translations in inference step")
	    ("only-violated-constraints", po::value<bool>(&onlyViolatedConstraints)->default_value(false), "Add only violated constraints to the optimisation problem")
	    ("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
	    ("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.9), "Adjust the factor for history smoothing")
	    ("use-scaled-reference", po::value<bool>(&useScaledReference)->default_value(true), "Use scaled reference length for comparing target and reference length of phrases")
	    ("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale the BLEU score by a history of the input lengths")
	    ("BP-factor", po::value<float>(&BPfactor)->default_value(1.0), "Increase penalty for short translations")
	    ("slack", po::value<float>(&slack)->default_value(0), "Use slack in optimizer")
	    ("slack-step", po::value<float>(&slack_step)->default_value(0), "Increase slack from epoch to epoch by the value provided")
	    ("slack-max", po::value<float>(&slack_max)->default_value(0), "Maximum slack used")
	    ("max-number-oracles", po::value<size_t>(&maxNumberOracles)->default_value(1), "Set a maximum number of oracles to use per example")
	    ("accumulate-most-violated-constraints", po::value<bool>(&accumulateMostViolatedConstraints)->default_value(false), "Accumulate most violated constraint per example")
	    ("past-and-current-constraints", po::value<bool>(&pastAndCurrentConstraints)->default_value(false), "Accumulate most violated constraint per example and use them along all current constraints")
	    ("suppress-convergence", po::value<bool>(&suppressConvergence)->default_value(false), "Suppress convergence, fixed number of epochs")
	    ("ignore-u-weight", po::value<bool>(&ignoreUWeight)->default_value(false), "Don't tune unknown word penalty weight")
	    ("ignore-weird-updates", po::value<bool>(&ignoreWeirdUpdates)->default_value(false), "Ignore updates that increase number of violated constraints AND increase the error")
	    ("log-feature-values", po::value<bool>(&logFeatureValues)->default_value(false), "Take log of feature values according to the given base.")
	    ("base-of-log", po::value<size_t>(&baseOfLog)->default_value(10), "Base for log-ing feature values")
	    ("clipping", po::value<float>(&clipping)->default_value(0.01), "Set a threshold to regularise updates")
	    ("fixed-clipping", po::value<bool>(&fixedClipping)->default_value(false), "Use a fixed clipping threshold")
	    ("decoder-settings",  po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
	    ("min-weight-change", po::value<float>(&min_weight_change)->default_value(0.01), "Set minimum weight change for stopping criterion")
	    ("dev-bleu", po::value<bool>(&devBleu)->default_value(false), "Compute BLEU score of oracle translations of the whole tuning set");

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);

  if (help) {
    std::cout << "Usage: " + string(argv[0]) +  " -f mosesini-file -i input-file -r reference-file(s) [options]" << std::endl;
    std::cout << desc << std::endl;
    return 0;
  }
  
  if (mosesConfigFile.empty()) {
    cerr << "Error: No moses ini file specified" << endl;
    return 1;
  }

  if (inputFile.empty()) {
    cerr << "Error: No input file specified" << endl;
    return 1;
  }

  if (!referenceFiles.size()) {
    cerr << "Error: No reference files specified" << endl;
    return 1;
  }

  // load input and references
  vector<string> inputSentences;
  if (!loadSentences(inputFile, inputSentences)) {
    cerr << "Error: Failed to load input sentences from " << inputFile << endl;
    return 1;
  }

  vector< vector<string> > referenceSentences(referenceFiles.size());
  for (size_t i = 0; i < referenceFiles.size(); ++i) {
    if (!loadSentences(referenceFiles[i], referenceSentences[i])) {
      cerr << "Error: Failed to load reference sentences from " << referenceFiles[i] << endl;
      return 1;
    }
    if (referenceSentences[i].size() != inputSentences.size()) {
      cerr << "Error: Input file length (" << inputSentences.size() <<
        ") != (" << referenceSentences[i].size() << ") length of reference file " << i  <<
          endl;
      return 1;
    }
  }

  // initialise Moses
  vector<string> decoder_params;
  boost::split(decoder_params, decoder_settings, boost::is_any_of("\t "));
  initMoses(mosesConfigFile, verbosity, decoder_params.size(), decoder_params);
  MosesDecoder* decoder = new MosesDecoder(referenceSentences, useScaledReference, scaleByInputLength, BPfactor, historySmoothing);
  ScoreComponentCollection startWeights = decoder->getWeights();
  startWeights.L1Normalise();
  decoder->setWeights(startWeights);

  // Optionally shuffle the sentences
  vector<size_t> order;
  if (rank == 0) {
	for (size_t i = 0; i < inputSentences.size(); ++i) {
      order.push_back(i);
    }

    if (shuffle) {
    	cerr << "Shuffling input sentences.." << endl;
    	RandomIndex rindex;
    	random_shuffle(order.begin(), order.end(), rindex);
    }
  }

#ifdef MPI_ENABLE
  mpi::broadcast(world, order, 0);
#endif

  // Create the shards according to the number of processes used
  vector<size_t> shard;
  float shardSize = (float)(order.size()) / size;
  VERBOSE(1, "Shard size: " << shardSize << endl);
  size_t shardStart = (size_t)(shardSize * rank);
  size_t shardEnd = (size_t)(shardSize * (rank+1));
  if (rank == size-1) shardEnd = order.size();
  VERBOSE(1, "Rank: " << rank << " Shard start: " << shardStart << " Shard end: " << shardEnd << endl);
  shard.resize(shardSize);
  copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());

  Optimiser* optimiser = NULL;
  cerr << "Nbest list size: " << n << endl;
  cerr << "Distinct translations in nbest list? " << distinctNbest << endl;
  cerr << "Batch size: " << batchSize << endl;
  cerr << "Maximum number of oracles: " << maxNumberOracles << endl;
  cerr << "Accumulate most violated constraints? " << accumulateMostViolatedConstraints << endl;
  cerr << "Margin scale factor: " << marginScaleFactor << endl;
  cerr << "Add only violated constraints? " << onlyViolatedConstraints << endl;
  cerr << "Using slack? " << slack << endl;
  cerr << "BP factor: " << BPfactor << endl;
  cerr << "Ignore unknown word penalty? " << ignoreUWeight << endl;
  cerr << "take log of feature values? " << logFeatureValues << endl;
  cerr << "base of log: " << baseOfLog << endl;
  cerr << "Fixed clipping? " << fixedClipping << endl;
  cerr << "clipping: " << clipping << endl;
  if (learner == "mira") {
    cerr << "Optimising using Mira" << endl;
    optimiser = new MiraOptimiser(n, hildreth, marginScaleFactor, onlyViolatedConstraints, clipping, fixedClipping, slack, weightedLossFunction, maxNumberOracles, accumulateMostViolatedConstraints, pastAndCurrentConstraints, order.size());
    if (hildreth) {
    	cerr << "Using Hildreth's optimisation algorithm.." << endl;
    }
    else {
    	cerr << "Using some sort of SMO.. " << endl;
    }

  } else if (learner == "perceptron") {
    cerr << "Optimising using Perceptron" << endl;
    optimiser = new Perceptron();
  } else {
    cerr << "Error: Unknown optimiser: " << learner << endl;
  }

  //Main loop:
  ScoreComponentCollection cumulativeWeights;		// collect weights per epoch to produce an average
  size_t weightChanges = 0;
  size_t weightChangesThisEpoch = 0;

  time_t now = time(0); // get current time
  struct tm* tm = localtime(&now); // get struct filled out
  cerr << "Start date/time: " << tm->tm_mon+1 << "/" << tm->tm_mday << "/" << tm->tm_year + 1900
		    << ", " << tm->tm_hour << ":" << tm->tm_min << ":" << tm->tm_sec << endl;
  
  // the result of accumulating and averaging weights over one epoch and possibly several processes
  ScoreComponentCollection averageTotalWeights;
  ScoreComponentCollection averageTotalWeightsCurrent;
  ScoreComponentCollection averageTotalWeightsPrevious;
  ScoreComponentCollection averageTotalWeightsBeforePrevious;

  // print initial weights
  cerr << "weights: " << decoder->getWeights() << endl;

  for (size_t epoch = 0; epoch < epochs; ++epoch) {
	  cerr << "\nEpoch " << epoch << endl;
	  weightChangesThisEpoch = 0;
	  // Sum up weights over one epoch, final average uses weights from last epoch
	  if (!accumulateWeights) {
		  cumulativeWeights.ZeroAll();
	  }

	  // number of weight dumps this epoch
	  size_t weightEpochDump = 0;

	  // collect all oracles for dev set
	  vector< vector< const Word*> > allOracles;
	  vector<size_t> all_ref_ids;

	  size_t shardPosition = 0;
	  vector<size_t>::const_iterator sid = shard.begin();
	  while (sid != shard.end()) {
		  // feature values for hypotheses i,j (matrix: batchSize x 3*n x featureValues)
		  vector<vector<ScoreComponentCollection > > featureValues;
		  vector<vector<float> > bleuScores;

		  // BATCHING: produce nbest lists for all input sentences in batch
		  vector<size_t> oraclePositions;
		  vector<float> oracleBleuScores;
		  vector< vector< const Word*> > oracles;
		  vector<ScoreComponentCollection> oracleFeatureValues;
		  vector<size_t> inputLengths;
		  vector<size_t> ref_ids;
		  size_t actualBatchSize = 0;
		  for (size_t batchPosition = 0; batchPosition < batchSize && sid != shard.end(); ++batchPosition) {
			  const string& input = inputSentences[*sid];
			  const vector<string>& refs = referenceSentences[*sid];
			  cerr << "\nRank " << rank << ", batch position " << batchPosition << endl;
			  cerr << "Rank " << rank << ", input sentence " << *sid << ": \"" << input << "\"" << endl;

			  vector<ScoreComponentCollection> newFeatureValues;
			  vector<float> newBleuScores;
			  featureValues.push_back(newFeatureValues);
			  bleuScores.push_back(newBleuScores);

			  // MODEL
			  cerr << "Rank " << rank << ", run decoder to get nbest wrt model score" << endl;
			  vector<const Word*> bestModel = decoder->getNBest(input,
                        *sid,
                        n,
                        0.0,
                        1.0,
                        featureValues[batchPosition],
                        bleuScores[batchPosition],
                        true,
                        distinctNbest,
                        ignoreUWeight,
                        rank);
			  inputLengths.push_back(decoder->getCurrentInputLength());
			  ref_ids.push_back(*sid);
			  all_ref_ids.push_back(*sid);
			  decoder->cleanup();
			  cerr << "Rank " << rank << ", model length: " << bestModel.size() << " Bleu: " << bleuScores[batchPosition][0] << endl;

			  // HOPE
			  cerr << "Rank " << rank << ", run decoder to get nbest hope translations" << endl;
			  size_t oraclePos = featureValues[batchPosition].size();
			  oraclePositions.push_back(oraclePos);
			  vector<const Word*> oracle = decoder->getNBest(input,
												*sid,
												n,
                        1.0,
                        1.0,
                        featureValues[batchPosition],
                        bleuScores[batchPosition],
                        true,
                        distinctNbest,
                        ignoreUWeight,
                        rank);
			  decoder->cleanup();
			  oracles.push_back(oracle);
			  allOracles.push_back(oracle);
			  cerr << "Rank " << rank << ", oracle length: " << oracle.size() << " Bleu: " << bleuScores[batchPosition][oraclePos] << endl;

			  oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
			  float oracleBleuScore = bleuScores[batchPosition][oraclePos];
			  oracleBleuScores.push_back(oracleBleuScore);

			  // FEAR
			  cerr << "Rank " << rank << ", run decoder to get nbest fear translations" << endl;
			  size_t fearPos = featureValues[batchPosition].size();
			  vector<const Word*> fear = decoder->getNBest(input,
                        *sid,
                        n,
                        -1.0,
                        1.0,
                        featureValues[batchPosition],
                        bleuScores[batchPosition],
                        true,
                        distinctNbest,
                        ignoreUWeight,
                        rank);
			  decoder->cleanup();
			  cerr << "Rank " << rank << ", fear length: " << fear.size() << " Bleu: " << bleuScores[batchPosition][fearPos] << endl;

			  for (size_t i = 0; i < bestModel.size(); ++i) {
				  delete bestModel[i];
			  }
			  for (size_t i = 0; i < fear.size(); ++i) {
				  delete fear[i];
			  }

			  cerr << "\nRank " << rank << ", sentence " << *sid << ", Bleu: " << bleuScores[batchPosition][oraclePos] << endl;

			  // next input sentence
			  ++sid;
			  ++actualBatchSize;
			  ++shardPosition;
		  }

		  // Set loss for each sentence as BLEU(oracle) - BLEU(hypothesis)
		  vector< vector<float> > losses(actualBatchSize);
		  for (size_t batchPosition = 0; batchPosition < actualBatchSize; ++batchPosition) {
		  	for (size_t j = 0; j < bleuScores[batchPosition].size(); ++j) {
		  		losses[batchPosition].push_back(oracleBleuScores[batchPosition] - bleuScores[batchPosition][j]);
		  	}
		  }

		  // get weight vector and set weight for bleu feature to 0
		  ScoreComponentCollection mosesWeights = decoder->getWeights();
		  const vector<const ScoreProducer*> featureFunctions = StaticData::Instance().GetTranslationSystem (TranslationSystem::DEFAULT).GetFeatureFunctions();
		  mosesWeights.Assign(featureFunctions.back(), 0);

		  if (!hildreth && typeid(*optimiser) == typeid(MiraOptimiser)) {
		  	((MiraOptimiser*)optimiser)->setOracleIndices(oraclePositions);
		  }

		  if (logFeatureValues) {
		  	for (size_t i = 0; i < featureValues.size(); ++i) {
		  		for (size_t j = 0; j < featureValues[i].size(); ++j) {
		  			featureValues[i][j].ApplyLog(baseOfLog);
		  		}

		  		oracleFeatureValues[i].ApplyLog(baseOfLog);
		  	}
		  }

		  // run optimiser on batch
		  cerr << "\nRank " << rank << ", run optimiser.." << endl;
		  ScoreComponentCollection oldWeights(mosesWeights);
		  int constraintChange = optimiser->updateWeights(mosesWeights, featureValues, losses, bleuScores, oracleFeatureValues, oracleBleuScores, ref_ids);

		  // normalise Moses weights
		  mosesWeights.L1Normalise();

		  // print weights and features values
		  cerr << "\nRank " << rank << ", weights (normalised): " << mosesWeights << endl;
/*		  cerr << "Rank " << rank << ", feature values: " << endl;
		  for (size_t i = 0; i < featureValues.size(); ++i) {
		  	for (size_t j = 0; j < featureValues[i].size(); ++j) {
		  		cerr << featureValues[i][j].Size() << ": " << featureValues[i][j] << endl;
		  	}
		  }
		  cerr << endl;*/

		  // compute difference to old weights
		  ScoreComponentCollection weightDifference(mosesWeights);
		  weightDifference.MinusEquals(oldWeights);
		  cerr << "Rank " << rank << ", weight difference: " << weightDifference << endl;
  
		  // update history (for approximate document Bleu)
		  for (size_t i = 0; i < oracles.size(); ++i) {
			  cerr << "Rank " << rank << ", oracle length: " << oracles[i].size() << " ";
		  }
		  decoder->updateHistory(oracles, inputLengths, ref_ids);

		  if (!devBleu) {
		  	// clean up oracle translations after updating history
		  	for (size_t i = 0; i < oracles.size(); ++i) {
		  		for (size_t j = 0; j < oracles[i].size(); ++j) {
		  			delete oracles[i][j];
		  		}
		  	}
		  }

		  // set and accumulate weights
		  decoder->setWeights(mosesWeights);
		  cumulativeWeights.PlusEquals(mosesWeights);

		  ++weightChanges;
		  ++weightChangesThisEpoch;

		  // mix weights?
#ifdef MPI_ENABLE
		  if (shardPosition % (shard.size() / mixFrequency) == 0) {
			  ScoreComponentCollection averageWeights;
			  if (rank == 0) {
			  	  cerr << "Rank 0, before mixing: " << mosesWeights << endl;
			  }

			  VERBOSE(1, "\nRank: " << rank << " \nBefore mixing: " << mosesWeights << endl);

			  // collect all weights in averageWeights and divide by number of processes
			  mpi::reduce(world, mosesWeights, averageWeights, SCCPlus(), 0);
			  if (rank == 0) {
				  averageWeights.DivideEquals(size);

				  // normalise weights after averaging
				  averageWeights.L1Normalise();

				  VERBOSE(1, "After mixing (normalised): " << averageWeights << endl);
				  cerr << "Rank 0, after mixing (normalised): " << averageWeights << endl;
			  }

			  // broadcast average weights from process 0
			  mpi::broadcast(world, averageWeights, 0);
			  decoder->setWeights(averageWeights);
		  }
#endif
		  // dump weights?
		  if (shardPosition % (shard.size() / weightDumpFrequency) == 0) {
			  // compute average weights per process over iterations
			  ScoreComponentCollection totalWeights(cumulativeWeights);
			  if (accumulateWeights)
				  totalWeights.DivideEquals(weightChanges);
			  else
				  totalWeights.DivideEquals(weightChangesThisEpoch);

			  if (rank == 0) {
			  	cerr << "Rank 0, cumulative weights: " << cumulativeWeights << endl;
			  	cerr << "Rank 0, total weights: " << totalWeights << endl;
			  }

			  if (weightEpochDump + 1 == weightDumpFrequency){
				  // last weight dump in epoch
				  averageTotalWeightsBeforePrevious = averageTotalWeightsPrevious;
				  averageTotalWeightsPrevious = averageTotalWeightsCurrent;
			  }

#ifdef MPI_ENABLE
			  // average across processes
			  mpi::reduce(world, totalWeights, averageTotalWeights, SCCPlus(), 0);
#endif
#ifndef MPI_ENABLE
			  // or use weights from single process
			  averageTotalWeights = totalWeights;
#endif

			  if (rank == 0 && !weightDumpStem.empty()) {
				  // average by number of processes and normalise weights
				  averageTotalWeights.DivideEquals(size);
				  averageTotalWeights.L1Normalise();
				  cerr << "Rank 0, average total weights (normalised): " << averageTotalWeights << endl;

				  ostringstream filename;
				  if (epoch < 10) {
					  filename << weightDumpStem << "_0" << epoch;
				  }
				  else {
					  filename << weightDumpStem << "_" << epoch; 	                                   
				  }

				  if (weightDumpFrequency > 1) {
					  filename << "_" << weightEpochDump;
				  }

				  VERBOSE(1, "Rank 0, dumping weights for epoch " << epoch << " to " << filename.str() << endl);
				  averageTotalWeights.Save(filename.str());

				  if (weightEpochDump + 1 == weightDumpFrequency){
					  // last weight dump in epoch
					  // compare new average weights with previous weights
					  averageTotalWeightsCurrent = averageTotalWeights;
					  ScoreComponentCollection firstDiff(averageTotalWeightsCurrent);
					  firstDiff.MinusEquals(averageTotalWeightsPrevious);
					  cerr << "Rank 0, weight changes since previous epoch: " << firstDiff << endl;
					  ScoreComponentCollection secondDiff(averageTotalWeightsCurrent);
					  secondDiff.MinusEquals(averageTotalWeightsBeforePrevious);
					  cerr << "Rank 0, weight changes since before previous epoch: " << secondDiff << endl;

					  if (!suppressConvergence) {
						  // check whether stopping criterion has been reached
						  // (both difference vectors must have all weight changes smaller than 0.01)
						  bool reached = true;
						  FVector changes1 = firstDiff.GetScoresVector();
						  FVector changes2 = secondDiff.GetScoresVector();
						  FVector::const_iterator iterator1 = changes1.cbegin();
						  FVector::const_iterator iterator2 = changes2.cbegin();
						  while (iterator1 != changes1.cend()) {
							  if (abs((*iterator1).second) >= min_weight_change || abs((*iterator2).second) >= min_weight_change) {
								  reached = false;
								  break;
							  }

							  ++iterator1;
							  ++iterator2;
						  }

						  if (reached)  {
							  // stop MIRA
							  cerr << "\nRank 0, stopping criterion has been reached after epoch " << epoch << ".. stopping MIRA." << endl;

							  ScoreComponentCollection dummy;
							  ostringstream endfilename;
							  endfilename << "stopping";
							  dummy.Save(endfilename.str());
							 
#ifdef MPI_ENABLE
							  MPI_Abort(MPI_COMM_WORLD, 0);
#endif
							  if (devBleu) {
							  	// calculate bleu score of all oracle translations of dev set
							  	decoder->calculateBleuOfCorpus(allOracles, all_ref_ids, epoch);
								}
							  if (marginScaleFactorStep > 0) {
							  	cerr << "margin scale factor: " << marginScaleFactor << endl;
							  }
							  if (slack_step > 0) {
							  	cerr << "slack: " << slack << endl;
							  }

							  goto end;
						  }
					  }
				  }

				  ++weightEpochDump;
			  }
		  }
	  }

	  if (devBleu) {
	  	// calculate bleu score of all oracle translations of dev set
	  	decoder->calculateBleuOfCorpus(allOracles, all_ref_ids, epoch);

	  	// clean up oracle translations
	  	for (size_t i = 0; i < allOracles.size(); ++i) {
	  		for (size_t j = 0; j < allOracles[i].size(); ++j) {
	  			delete allOracles[i][j];
	  		}
	  	}
	  }

	  // if using flexible margin scale factor, increase scaling (decrease value) for next epoch
	  if (marginScaleFactorStep > 0) {
	  	if (marginScaleFactor - marginScaleFactorStep >= marginScaleFactorMin) {
	  		if (typeid(*optimiser) == typeid(MiraOptimiser)) {
	  			cerr << "old margin scale factor: " << marginScaleFactor << endl;
					marginScaleFactor -= marginScaleFactorStep;
					cerr << "new margin scale factor: " << marginScaleFactor << endl;
					((MiraOptimiser*)optimiser)->setMarginScaleFactor(marginScaleFactor);
	  		}
	  	}
	  	else {
	  		cerr << "margin scale factor: " << marginScaleFactor << endl;
	  	}
	  }

	  // if using flexible slack, increase slack for next epoch
	  if (slack_step > 0) {
	  	if (slack + slack_step <= slack_max) {
	  		if (typeid(*optimiser) == typeid(MiraOptimiser)) {
	  			cerr << "old slack: " << slack << endl;
	  			slack += slack_step;
	  			cerr << "new slack: " << slack << endl;
	  			((MiraOptimiser*)optimiser)->setSlack(slack);
	  		}
	  	}
	  	else {
	  		cerr << "slack: " << slack << endl;
	  	}
	  }
  }

 end:  

#ifdef MPI_ENABLE
  MPI_Finalize();
#endif

  now = time(0); // get current time
  tm = localtime(&now); // get struct filled out
  cerr << "\nEnd date/time: " << tm->tm_mon+1 << "/" << tm->tm_mday << "/" << tm->tm_year + 1900
		    << ", " << tm->tm_hour << ":" << tm->tm_min << ":" << tm->tm_sec << endl;

  delete decoder;
  exit(0);
}

