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
  size_t mixingFrequency;
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
  bool weightConvergence;
  bool controlUpdates;
  float learning_rate;
  bool logFeatureValues;
  size_t baseOfLog;
  string decoder_settings;
  float min_weight_change;
  float max_sentence_update;
  float decrease_learning_rate;
  float decrease_sentence_update;
  bool devBleu;
  bool normaliseWeights;
  bool print_feature_values;
  bool stop_dev_bleu;
  bool stop_approx_dev_bleu;
  po::options_description desc("Allowed options");
  desc.add_options()
		  ("accumulate-most-violated-constraints", po::value<bool>(&accumulateMostViolatedConstraints)->default_value(false), "Accumulate most violated constraint per example")
		  ("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(true), "Accumulate and average weights over all epochs")
		  ("base-of-log", po::value<size_t>(&baseOfLog)->default_value(10), "Base for log-ing feature values")
		  ("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
    	("BP-factor", po::value<float>(&BPfactor)->default_value(1.0), "Increase penalty for short translations")
      ("config,f",po::value<string>(&mosesConfigFile),"Moses ini file")
      ("control-updates", po::value<bool>(&controlUpdates)->default_value(false), "Ignore updates that increase number of violated constraints AND increase the error")
      ("decoder-settings",  po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
      ("decr-learning-rate", po::value<float>(&decrease_learning_rate)->default_value(0), "Decrease learning rate by the given value after every epoch")
      ("decr-sentence-update", po::value<float>(&decrease_sentence_update)->default_value(0), "Decrease maximum weight update by the given value after every epoch")
      ("dev-bleu", po::value<bool>(&devBleu)->default_value(true), "Compute BLEU score of oracle translations of the whole tuning set")
      ("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(false), "Use nbest list with distinct translations in inference step")
      ("epochs,e", po::value<size_t>(&epochs)->default_value(5), "Number of epochs")
      ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
      ("hildreth", po::value<bool>(&hildreth)->default_value(true), "Use Hildreth's optimisation algorithm")
      ("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.9), "Adjust the factor for history smoothing")
      ("input-file,i",po::value<string>(&inputFile),"Input file containing tokenised source")
      ("learner,l", po::value<string>(&learner)->default_value("mira"), "Learning algorithm")
      ("learning-rate", po::value<float>(&learning_rate)->default_value(1), "Learning rate (fixed or flexible)")
      ("log-feature-values", po::value<bool>(&logFeatureValues)->default_value(false), "Take log of feature values according to the given base.")
      ("max-number-oracles", po::value<size_t>(&maxNumberOracles)->default_value(1), "Set a maximum number of oracles to use per example")
      ("max-sentence-update", po::value<float>(&max_sentence_update)->default_value(0), "Set a maximum weight update per sentence")
      ("min-weight-change", po::value<float>(&min_weight_change)->default_value(0.01), "Set minimum weight change for stopping criterion")
      ("mixing-frequency", po::value<size_t>(&mixingFrequency)->default_value(1), "How often per epoch to mix weights, when using mpi")
	    ("msf", po::value<float>(&marginScaleFactor)->default_value(1.0), "Margin scale factor, regularises the update by scaling the enforced margin")
	    ("msf-min", po::value<float>(&marginScaleFactorMin)->default_value(1.0), "Minimum value that margin is scaled by")
	    ("msf-step", po::value<float>(&marginScaleFactorStep)->default_value(0), "Decrease margin scale factor iteratively by the value provided")
	    ("nbest,n", po::value<size_t>(&n)->default_value(10), "Number of translations in nbest list")
	    ("normalise", po::value<bool>(&normaliseWeights)->default_value(false), "Whether to normalise the updated weights before passing them to the decoder")
	    ("only-violated-constraints", po::value<bool>(&onlyViolatedConstraints)->default_value(false), "Add only violated constraints to the optimisation problem")
	    ("past-and-current-constraints", po::value<bool>(&pastAndCurrentConstraints)->default_value(false), "Accumulate most violated constraint per example and use them along all current constraints")
	    ("print-feature-values", po::value<bool>(&print_feature_values)->default_value(false), "Print out feature values")
	    ("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training")
	    ("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale the BLEU score by a history of the input lengths")
	    ("shuffle", po::value<bool>(&shuffle)->default_value(false), "Shuffle input sentences before processing")
	    ("slack", po::value<float>(&slack)->default_value(0.01), "Use slack in optimizer")
	    ("slack-max", po::value<float>(&slack_max)->default_value(0), "Maximum slack used")
	    ("slack-step", po::value<float>(&slack_step)->default_value(0), "Increase slack from epoch to epoch by the value provided")
	    ("stop-dev-bleu", po::value<bool>(&stop_dev_bleu)->default_value(false), "Stop when average Bleu (dev) decreases")
	    ("stop-approx-dev-bleu", po::value<bool>(&stop_approx_dev_bleu)->default_value(false), "Stop when average approx. sentence Bleu (dev) decreases")
	    ("stop-weights", po::value<bool>(&weightConvergence)->default_value(false), "Stop when weights converge")
	    ("use-scaled-reference", po::value<bool>(&useScaledReference)->default_value(true), "Use scaled reference length for comparing target and reference length of phrases")
	    ("verbosity,v", po::value<int>(&verbosity)->default_value(0), "Verbosity level")
	    ("weighted-loss-function", po::value<bool>(&weightedLossFunction)->default_value(false), "Weight the loss of a hypothesis by its Bleu score")
	    ("weight-dump-stem", po::value<string>(&weightDumpStem)->default_value("weights"), "Stem of filename to use for dumping weights");

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
  if (normaliseWeights) {
  	ScoreComponentCollection startWeights = decoder->getWeights();
  	startWeights.L1Normalise();
  	decoder->setWeights(startWeights);
  }

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
  cerr << "mix-frequency: " << mixingFrequency << endl;
  cerr << "weight-dump-stem: " << weightDumpStem << endl;
  cerr << "shuffle: " << shuffle << endl;
  cerr << "hildreth: " << hildreth << endl;
  cerr << "msf: " << marginScaleFactor << endl;
  cerr << "msf-step: " << marginScaleFactorStep << endl;
  cerr << "msf-min: " << marginScaleFactorMin << endl;
  cerr << "weighted-loss-function: " << weightedLossFunction << endl;
  cerr << "nbest: " << n << endl;
  cerr << "batch-size: " << batchSize << endl;
  cerr << "distinct-nbest: " << distinctNbest << endl;
  cerr << "only-violated-constraints: " << onlyViolatedConstraints << endl;
  cerr << "accumulate-weights: " << accumulateWeights << endl;
  cerr << "history-smoothing: " << historySmoothing << endl;
  cerr << "use-scaled-reference: " << useScaledReference << endl;
  cerr << "scale-by-input-length: " << scaleByInputLength << endl;
  cerr << "BP-factor: " << BPfactor << endl;
  cerr << "slack: " << slack << endl;
  cerr << "slack-step: " << slack_step << endl;
  cerr << "slack-max: " << slack_max << endl;
  cerr << "max-number-oracles: " << maxNumberOracles << endl;
  cerr << "accumulate-most-violated-constraints: " << accumulateMostViolatedConstraints << endl;
  cerr << "past-and-current-constraints: " << pastAndCurrentConstraints << endl;
  cerr << "control-updates: " << controlUpdates << endl;
  cerr << "log-feature-values: " << logFeatureValues << endl;
  cerr << "base-of-log: " << baseOfLog << endl;
  cerr << "decoder-settings: " << decoder_settings << endl;
  cerr << "min-weight-change: " << min_weight_change << endl;
  cerr << "max-sentence-update: " << max_sentence_update << endl;
  cerr << "decr-learning-rate: " << decrease_learning_rate << endl;
  cerr << "dev-bleu: " << devBleu << endl;
  cerr << "normalise: " << normaliseWeights << endl;
  cerr << "print-feature-values: " << print_feature_values << endl;
  cerr << "stop-dev-bleu: " << stop_dev_bleu << endl;
  cerr << "stop-approx-dev-bleu: " << stop_approx_dev_bleu << endl;
  cerr << "stop-weights: " << weightConvergence << endl;


  if (learner == "mira") {
    cerr << "Optimising using Mira" << endl;
    optimiser = new MiraOptimiser(n, hildreth, marginScaleFactor, onlyViolatedConstraints, slack, weightedLossFunction, maxNumberOracles, accumulateMostViolatedConstraints, pastAndCurrentConstraints, order.size());
    if (hildreth) {
    	cerr << "Using Hildreth's optimisation algorithm.." << endl;
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
  
  ScoreComponentCollection averageWeights;
  ScoreComponentCollection averageTotalWeights;
  ScoreComponentCollection averageTotalWeightsEnd;
  ScoreComponentCollection averageTotalWeightsPrevious;
  ScoreComponentCollection averageTotalWeightsBeforePrevious;

  // print initial weights
  cerr << "weights: " << decoder->getWeights() << endl;

  float averageBleu = 0;
  float prevAverageBleu = 0;
  float beforePrevAverageBleu = 0;
  float summedApproxBleu = 0;
  float averageApproxBleu = 0;
  float prevAverageApproxBleu = 0;
  float beforePrevAverageApproxBleu = 0;
  bool stop = false;
  float *sendbuf, *recvbuf;
  sendbuf = (float *)malloc(sizeof(float));
  recvbuf = (float *)malloc(sizeof(float));
  for (size_t epoch = 0; epoch < epochs && !stop; ++epoch) {
	  cerr << "\nEpoch " << epoch << endl;
	  weightChangesThisEpoch = 0;
	  summedApproxBleu = 0;
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
                        rank);
			  decoder->cleanup();
			  cerr << "Rank " << rank << ", fear length: " << fear.size() << " Bleu: " << bleuScores[batchPosition][fearPos] << endl;

			  for (size_t i = 0; i < bestModel.size(); ++i) {
				  delete bestModel[i];
			  }
			  for (size_t i = 0; i < fear.size(); ++i) {
				  delete fear[i];
			  }

			  cerr << "Sentence " << *sid << ", Bleu: " << bleuScores[batchPosition][oraclePos] << endl;
			  summedApproxBleu += bleuScores[batchPosition][oraclePos];

			  // next input sentence
			  ++sid;
			  ++actualBatchSize;
			  ++shardPosition;
		  } // end of batch loop

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
		  int updateStatus = optimiser->updateWeights(mosesWeights, featureValues, losses, bleuScores, oracleFeatureValues, oracleBleuScores, ref_ids, learning_rate, max_sentence_update);

		  // set decoder weights and accumulate weights
		  if (controlUpdates && updateStatus < 0) {
		  	// TODO: could try to repeat hildreth with more slack
		  	cerr << "update ignored!" << endl;
		  }
		  else {
		  	if (normaliseWeights) {
		  		mosesWeights.L1Normalise();
		  		cerr << "\nRank " << rank << ", weights (normalised): " << mosesWeights << endl;
		  	}
		  	else {
		  		cerr << "\nRank " << rank << ", weights: " << mosesWeights << endl;
		  	}

		  	decoder->setWeights(mosesWeights);
		  	cumulativeWeights.PlusEquals(mosesWeights);

		  	++weightChanges;
		  	++weightChangesThisEpoch;

		  	// compute difference to old weights
		  	ScoreComponentCollection weightDifference(mosesWeights);
		  	weightDifference.MinusEquals(oldWeights);
		  	cerr << "Rank " << rank << ", weight difference: " << weightDifference << endl;
		  }

		  if (print_feature_values) {
		  	cerr << "Rank " << rank << ", feature values: " << endl;
				for (size_t i = 0; i < featureValues.size(); ++i) {
					for (size_t j = 0; j < featureValues[i].size(); ++j) {
						cerr << featureValues[i][j].Size() << ": " << featureValues[i][j] << endl;
					}
				}
				cerr << endl;
		  }

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

		  // mix weights?
		  if (shardPosition % (shard.size() / mixingFrequency) == 0) {
		  	ScoreComponentCollection averageWeights;
#ifdef MPI_ENABLE
			  cerr << "\nRank " << rank << ", before mixing: " << mosesWeights << endl;

			  // collect all weights in averageWeights and divide by number of processes
			  mpi::reduce(world, mosesWeights, averageWeights, SCCPlus(), 0);
			  if (rank == 0) {
			  	// divide by number of processes
				  averageWeights.DivideEquals(size);

				  // normalise weights after averaging
				  if (normaliseWeights) {
				  	averageWeights.L1Normalise();
				  	cerr << "Average weights after mixing (normalised): " << averageWeights << endl;
				  }
				  else {
				  	cerr << "Average weights after mixing: " << averageWeights << endl;
				  }
			  }

			  // broadcast average weights from process 0
			  mpi::broadcast(world, averageWeights, 0);
			  decoder->setWeights(averageWeights);
#endif
#ifndef MPI_ENABLE
			  averageWeights = mosesWeights;
#endif
			  // dump weights after mixing
			  if (rank == 0 && !weightDumpStem.empty()) {
			  	ostringstream filename;
			  	if (epoch < 10) {
			  		filename << weightDumpStem << "_0" << epoch;
			  	}
			  	else {
			  		filename << weightDumpStem << "_" << epoch;
			  	}

			  	if (mixingFrequency > 1) {
			  		filename << "_" << weightEpochDump << "_mixed";
			  	}

			  	cerr << "Dumping average weights for epoch " << epoch << " to " << filename.str() << endl;
			  	averageWeights.Save(filename.str());
			  }

			  // Average and dump weights of all processes over one or more epochs
			  ScoreComponentCollection totalWeights(cumulativeWeights);
			  if (accumulateWeights) {
			  	totalWeights.DivideEquals(weightChanges);
			  }
			  else {
			  	totalWeights.DivideEquals(weightChangesThisEpoch);
			  }

#ifdef MPI_ENABLE
			  // average across processes
			  mpi::reduce(world, totalWeights, averageTotalWeights, SCCPlus(), 0);
#endif
#ifndef MPI_ENABLE
			  averageTotalWeights = totalWeights;
#endif
			  if (rank == 0 && !weightDumpStem.empty()) {
			  	// divide by number of processes
			  	averageTotalWeights.DivideEquals(size);

			  	// normalise weights after averaging
			  	if (normaliseWeights) {
			  		averageTotalWeights.L1Normalise();
			  	}

			  	// dump final average weights
			  	ostringstream filename;
			  	if (epoch < 10) {
			  		filename << weightDumpStem << "_0" << epoch;
			  	}
			  	else {
			  		filename << weightDumpStem << "_" << epoch;
			  	}

			  	if (mixingFrequency > 1) {
							filename << "_" << weightEpochDump;
			  	}

			  	if (accumulateWeights) {
			  		cerr << "\nAverage total weights (cumulative) after epoch " << epoch << ": " << averageTotalWeights << endl;
			  	}
			  	else {
			  		cerr << "\nAverage total weights after epoch " << epoch << ": " << averageTotalWeights << endl;
			  	}

			  	cerr << "Dumping average total weights after epoch " << epoch << " to " << filename.str() << endl;
			  	averageTotalWeights.Save(filename.str());
			  	++weightEpochDump;
			  }// end averaging and printing total weights
		  } //end mixing
	  } // end of shard loop, end of this epoch

	  if (devBleu) {
	  	// calculate bleu score of all oracle translations of dev set
	  	float bleu = decoder->calculateBleuOfCorpus(allOracles, all_ref_ids, epoch, rank);

	  	// print out translations
	  	ostringstream filename;
	  	if (epoch < 10) {
	  		filename << "dev_set_oracles" << "_0" << epoch << "_rank" << rank;
	  	}
	  	else {
	  		filename << "dev_set_oracles" << "_" << epoch << "_rank" << rank;
	  	}
	  	ofstream out((filename.str()).c_str());

	  	// print oracle translations to file and delete them afterwards
	  	if (!out) {
	  		ostringstream msg;
	  		msg << "Unable to open " << filename;
	  		throw runtime_error(msg.str());
	  	}
	  	else {
	  		for (size_t i = 0; i < allOracles.size(); ++i) {
	  			for (size_t j = 0; j < allOracles[i].size(); ++j) {
	  				out << *(allOracles[i][j]);
	  				delete allOracles[i][j];
	  			}
	  			out << endl;
	  		}
	  		out.close();
	  	}

	  	if (rank == 0) {
	  		beforePrevAverageBleu = prevAverageBleu;
				beforePrevAverageApproxBleu = prevAverageApproxBleu;
				prevAverageBleu = averageBleu;
				prevAverageApproxBleu = averageApproxBleu;
	  	}

#ifdef MPI_ENABLE
	  	// average bleu across processes
	  	sendbuf[0] = bleu;
	  	recvbuf[0] = 0;
	  	MPI_Reduce(sendbuf, recvbuf, 1, MPI_FLOAT, MPI_SUM, 0, world);
	  	if (rank == 0) {
	  		averageBleu = recvbuf[0];

	  		// divide by number of processes
	  		averageBleu /= size;
	  		cerr << "Average Bleu (dev) after epoch " << epoch << ": " << averageBleu << endl;
	  	}

	  	// average approximate sentence bleu across processes
	  	sendbuf[0] = summedApproxBleu/weightChangesThisEpoch;
	  	recvbuf[0] = 0;
	  	MPI_Reduce(sendbuf, recvbuf, 1, MPI_FLOAT, MPI_SUM, 0, world);
	  	if (rank == 0) {
	  		averageApproxBleu = recvbuf[0];

	  		// divide by number of processes
	  		averageApproxBleu /= size;
	  		cerr << "Average approx. sentence Bleu (dev) after epoch " << epoch << ": " << averageApproxBleu << endl;
	  	}
#endif
#ifndef MPI_ENABLE
	  	averageBleu = bleu;
	  	cerr << "Average Bleu (dev) after epoch " << epoch << ": " << averageBleu << endl;
	  	averageApproxBleu = summedApproxBleu/weightChangesThisEpoch;
	  	cerr << "Average approx. sentence Bleu (dev) after epoch " << epoch << ": " << averageApproxBleu << endl;
#endif

	  	if (rank == 0) {
	  		if (averageBleu < prevAverageBleu && prevAverageBleu < beforePrevAverageBleu) {
	  			if (stop_dev_bleu) {
	  				stop = true;
	  				cerr << "Average Bleu (dev) is decreasing.. stop tuning." << endl;
	  				ScoreComponentCollection dummy;
	  				ostringstream endfilename;
	  				endfilename << "stopping";
	  				dummy.Save(endfilename.str());
	  			}
	  			else
	  				cerr << "Average Bleu (dev) is decreasing.." << endl;
	  		}

	  		if (averageApproxBleu < prevAverageApproxBleu && prevAverageApproxBleu < beforePrevAverageApproxBleu) {
	  			if (stop_approx_dev_bleu) {
	  				stop = true;
	  				cerr << "Average approx. sentence Bleu (dev) is decreasing.. stop tuning." << endl;
	  				ScoreComponentCollection dummy;
	  				ostringstream endfilename;
	  				endfilename << "stopping";
	  				dummy.Save(endfilename.str());
	  			}
	  			else
	  				cerr << "Average approx. sentence Bleu (dev) is decreasing.." << endl;
	  		}
	  	}

#ifdef MPI_ENABLE
	  	mpi::broadcast(world, stop, 0);
#endif
	  } // end if (dev_bleu)

	  // Average weights of all processes over one or more epochs
	  ScoreComponentCollection totalWeights(cumulativeWeights);
	  if (accumulateWeights) {
	  	totalWeights.DivideEquals(weightChanges);
	  }
	  else {
	  	totalWeights.DivideEquals(weightChangesThisEpoch);
	  }

	  if (rank == 0) {
				averageTotalWeightsBeforePrevious = averageTotalWeightsPrevious;
				averageTotalWeightsPrevious = averageTotalWeightsEnd;
	  }
#ifdef MPI_ENABLE
	  // average across processes
	  mpi::reduce(world, totalWeights, averageTotalWeightsEnd, SCCPlus(), 0);
#endif
#ifndef MPI_ENABLE
	  averageTotalWeightsEnd = totalWeights;
#endif
	  if (rank == 0 && !weightDumpStem.empty()) {
	  	// divide by number of processes
	  	averageTotalWeightsEnd.DivideEquals(size);

	  	// normalise weights after averaging
	  	if (normaliseWeights) {
	  		averageTotalWeightsEnd.L1Normalise();
	  	}

	  	// dump final average weights
	  	ostringstream filename;
	  	if (epoch < 10) {
	  		filename << weightDumpStem << "_0" << epoch << "_averageTotal";
	  	}
	  	else {
	  		filename << weightDumpStem << "_" << epoch << "_averageTotal";
	  	}

	  	if (accumulateWeights) {
	  		cerr << "\nAverage total weights (cumulative) after epoch " << epoch << ": " << averageTotalWeightsEnd << endl;
	  	}
	  	else {
	  		cerr << "\nAverage total weights after epoch " << epoch << ": " << averageTotalWeightsEnd << endl;
	  	}

	  	cerr << "Dumping average total weights after epoch " << epoch << " to " << filename.str() << endl;
	  	averageTotalWeightsEnd.Save(filename.str());

	  }// end averaging and printing total weights

	  // Test if weights have converged
	  if (weightConvergence) {
	  	bool reached = true;
	  	if (rank == 0) {
	    	ScoreComponentCollection firstDiff(averageTotalWeights);
	    	firstDiff.MinusEquals(averageTotalWeightsPrevious);
	    	cerr << "Average weight changes since previous epoch: " << firstDiff << endl;
	    	ScoreComponentCollection secondDiff(averageTotalWeights);
	    	secondDiff.MinusEquals(averageTotalWeightsBeforePrevious);
	    	cerr << "Average weight changes since before previous epoch: " << secondDiff << endl << endl;

	    	// check whether stopping criterion has been reached
	    	// (both difference vectors must have all weight changes smaller than min_weight_change)
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
	    		stop = true;
	    		cerr << "Stopping criterion has been reached after epoch " << epoch << ".. stopping MIRA." << endl;
	    		ScoreComponentCollection dummy;
	    		ostringstream endfilename;
	    		endfilename << "stopping";
	    		dummy.Save(endfilename.str());
	    	}
	  	}
#ifdef MPI_ENABLE
	  	mpi::broadcast(world, stop, 0);
#endif
	  } //end if (weightConvergence)

	  // if using flexible margin scale factor, increase scaling (decrease value) for next epoch
	  if (marginScaleFactorStep > 0) {
	  	if (marginScaleFactor - marginScaleFactorStep >= marginScaleFactorMin) {
	  		if (typeid(*optimiser) == typeid(MiraOptimiser)) {
	  			marginScaleFactor -= marginScaleFactorStep;
	  			cerr << "Change margin scale factor to: " << marginScaleFactor << endl;
	  			((MiraOptimiser*)optimiser)->setMarginScaleFactor(marginScaleFactor);
	  		}
	  	}
	  }

	  // if using flexible slack, increase slack for next epoch
	  if (slack_step > 0) {
	  	if (slack + slack_step <= slack_max) {
	  		if (typeid(*optimiser) == typeid(MiraOptimiser)) {
	  			slack += slack_step;
	  			cerr << "Change slack to: " << slack << endl;
	  			((MiraOptimiser*)optimiser)->setSlack(slack);
	  		}
	  	}
	  }

	  // change learning rate
	  if ((decrease_learning_rate > 0) && (learning_rate - decrease_learning_rate > 0)) {
	  	learning_rate -= decrease_learning_rate;
	  	cerr << "Change learning rate to " << learning_rate << endl;
	  }

	  // change maximum sentence update
	  if ((decrease_sentence_update > 0) && (max_sentence_update - decrease_sentence_update > 0)) {
	  	max_sentence_update -= decrease_sentence_update;
	  	cerr << "Change maximum sentence update to " << max_sentence_update << endl;
	  }
  } // end of epoch loop

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

