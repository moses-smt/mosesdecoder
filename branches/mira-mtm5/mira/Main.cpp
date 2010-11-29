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
  size_t n;
  size_t batchSize;
  bool distinctNbest;
  bool onlyViolatedConstraints;
  bool accumulateWeights;
  bool useScaledReference;
  bool scaleByInputLength;
  bool increaseBP;
  bool regulariseHildrethUpdates;
  float clipping;
  bool fixedClipping;
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
	    ("margin-scale-factor,m", po::value<float>(&marginScaleFactor)->default_value(1.0), "Margin scale factor, regularises the update by scaling the enforced margin")
	    ("nbest,n", po::value<size_t>(&n)->default_value(10), "Number of translations in nbest list")
	    ("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
	    ("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(false), "Use nbest list with distinct translations in inference step")
	    ("only-violated-constraints", po::value<bool>(&onlyViolatedConstraints)->default_value(false), "Add only violated constraints to the optimisation problem")
	    ("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
	    ("use-scaled-reference", po::value<bool>(&useScaledReference)->default_value(true), "Use scaled reference length for comparing target and reference length of phrases")
	    ("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale the BLEU score by a history of the input lengths")
	    ("increase-BP", po::value<bool>(&increaseBP)->default_value(false), "Increase penalty for short translations")
	    ("regularise-hildreth-updates", po::value<bool>(&regulariseHildrethUpdates)->default_value(false), "Regularise Hildreth updates with the value set for clipping")
	    ("clipping", po::value<float>(&clipping)->default_value(0.01f), "Set a threshold to regularise updates")
	    ("fixed-clipping", po::value<bool>(&fixedClipping)->default_value(false), "Use a fixed clipping threshold with SMO (instead of adaptive)");


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
  initMoses(mosesConfigFile, verbosity);//, argc, argv);
  MosesDecoder* decoder = new MosesDecoder(referenceSentences, useScaledReference, scaleByInputLength, increaseBP);
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
  if (learner == "mira") {
    cerr << "Optimising using Mira" << endl;
    optimiser = new MiraOptimiser(n, hildreth, marginScaleFactor, onlyViolatedConstraints, clipping, fixedClipping, regulariseHildrethUpdates);
    if (hildreth) {
    	cerr << "Using Hildreth's optimisation algorithm.." << endl;
    }
    else {
    	cerr << "Using some sort of SMO.. " << endl;
    }

    cerr << "Margin scale factor: " << marginScaleFactor << endl;
    cerr << "Add only violated constraints? " << onlyViolatedConstraints << endl;
  } else if (learner == "perceptron") {
    cerr << "Optimising using Perceptron" << endl;
    optimiser = new Perceptron();
  } else {
    cerr << "Error: Unknown optimiser: " << learner << endl;
  }

  //Main loop:
  ScoreComponentCollection cumulativeWeights;		// collect weights per epoch to produce an average
  size_t iterations = 0;
  size_t iterationsThisEpoch = 0;

  time_t now = time(0); // get current time
  struct tm* tm = localtime(&now); // get struct filled out
  cerr << "Start date/time: " << tm->tm_mon+1 << "/" << tm->tm_mday << "/" << tm->tm_year + 1900
		    << ", " << tm->tm_hour << ":" << tm->tm_min << ":" << tm->tm_sec << endl;
  
  // the result of accumulating and averaging weights over one epoch and possibly several processes
  ScoreComponentCollection averageTotalWeights;

  // TODO: scaling of feature values for probabilistic features
  for (size_t epoch = 0; epoch < epochs; ++epoch) {
	  cerr << "\nEpoch " << epoch << endl;
	  // Sum up weights over one epoch, final average uses weights from last epoch
	  iterationsThisEpoch = 0;
	  if (!accumulateWeights) {
		  cumulativeWeights.ZeroAll();
	  }

	  // number of weight dumps this epoch
	  size_t weightEpochDump = 0;

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
			  cerr << "\nBatch position " << batchPosition << endl;
			  cerr << "Input sentence " << *sid << ": \"" << input << "\"" << endl;

			  vector<ScoreComponentCollection> newFeatureValues;
			  vector<float> newBleuScores;
			  featureValues.push_back(newFeatureValues);
			  bleuScores.push_back(newBleuScores);

			  // MODEL
			  cerr << "Run decoder to get nbest wrt model score" << endl;
			  vector<const Word*> bestModel = decoder->getNBest(input,
                        *sid,
                        n,
                        0.0,
                        1.0,
                        featureValues[batchPosition],
                        bleuScores[batchPosition],
                        true,
                        distinctNbest);
			  inputLengths.push_back(decoder->getCurrentInputLength());
			  ref_ids.push_back(*sid);
			  decoder->cleanup();
			  for (size_t i = 0; i < bestModel.size(); ++i) {
				  cerr << *(bestModel[i]) << " ";
			  }
			  cerr << endl;
			  cerr << "model length: " << bestModel.size() << " Bleu: " << bleuScores[batchPosition][0] << endl;

			  // HOPE
			  cerr << "Run decoder to get nbest hope translations" << endl;
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
                        distinctNbest);
			  decoder->cleanup();
			  oracles.push_back(oracle);
			  for (size_t i = 0; i < oracle.size(); ++i) {
				  //oracles[batchPosition].push_back(oracle[i]);
				  cerr << *(oracle[i]) << " ";
			  }
			  cerr << endl;
			  cerr << "oracle length: " << oracle.size() << " Bleu: " << bleuScores[batchPosition][oraclePos] << endl;

			  oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
			  float oracleBleuScore = bleuScores[batchPosition][oraclePos];
			  oracleBleuScores.push_back(oracleBleuScore);

			  // FEAR
			  cerr << "Run decoder to get nbest fear translations" << endl;
			  size_t fearPos = featureValues[batchPosition].size();
			  vector<const Word*> fear = decoder->getNBest(input,
                        *sid,
                        n,
                        -1.0,
                        1.0,
                        featureValues[batchPosition],
                        bleuScores[batchPosition],
                        true,
                        distinctNbest);
			  decoder->cleanup();
			  for (size_t i = 0; i < fear.size(); ++i) {
				  cerr << *(fear[i]) << " ";
			  }
			  cerr << endl;
			  cerr << "fear length: " << fear.size() << " Bleu: " << bleuScores[batchPosition][fearPos] << endl;

			  for (size_t i = 0; i < bestModel.size(); ++i) {
				  delete bestModel[i];
			  }
			  for (size_t i = 0; i < fear.size(); ++i) {
				  delete fear[i];
			  }

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

		  // run optimiser on batch
	      cerr << "\nRun optimiser.." << endl;
	      ScoreComponentCollection oldWeights(mosesWeights);
	      size_t constraintChange = optimiser->updateWeights(mosesWeights, featureValues, losses, oracleFeatureValues);

		  // update moses weights
	      mosesWeights.L1Normalise();
		  decoder->setWeights(mosesWeights);
  
		  // update history (for approximate document bleu)
		  decoder->updateHistory(oracles, inputLengths, ref_ids);

		  // clean up oracle translations after updating history
		  for (size_t i = 0; i < oracles.size(); ++i) {
			  for (size_t j = 0; j < oracles[i].size(); ++j) {
				  delete oracles[i][j];
			  }
		  }

		  cumulativeWeights.PlusEquals(mosesWeights);

		  // sanity check: compare margin created by old weights against new weights
		  float lossMinusMargin_old = 0;
		  float lossMinusMargin_new = 0;
		  for (size_t batchPosition = 0; batchPosition < actualBatchSize; ++batchPosition) {
			  for (size_t j = 0; j < featureValues[batchPosition].size(); ++j) {
				  ScoreComponentCollection featureDiff(oracleFeatureValues[batchPosition]);
				  featureDiff.MinusEquals(featureValues[batchPosition][j]);

				  // old weights
				  float margin = featureDiff.InnerProduct(oldWeights);
				  lossMinusMargin_old += (losses[batchPosition][j] - margin);
				  // new weights
				  margin = featureDiff.InnerProduct(mosesWeights);
				  lossMinusMargin_new += (losses[batchPosition][j] - margin);
			  }
	      }

		  cerr << "\nConstraint change: " << constraintChange << endl;
	      cerr << "Summed (loss - margin) with old weights: " << lossMinusMargin_old << endl;
	      cerr << "Summed (loss - margin) with new weights: " << lossMinusMargin_new << endl;
	      if (lossMinusMargin_new > lossMinusMargin_old) {
	    	  cerr << "Worsening: " << lossMinusMargin_new - lossMinusMargin_old << endl;

	    	  if (constraintChange < 0) {
	    		  cerr << "Something is going wrong here.." << endl;
	    	  }
	      }

	      ++iterations;
		  ++iterationsThisEpoch;

		  // mix weights?
#ifdef MPI_ENABLE
		  if (shardPosition % (shard.size() / mixFrequency) == 0) {
			  ScoreComponentCollection averageWeights;
			  VERBOSE(1, "\nRank: " << rank << " \nBefore mixing: " << mosesWeights << endl);

			  // collect all weights in averageWeights and divide by number of processes
			  mpi::reduce(world, mosesWeights, averageWeights, SCCPlus(), 0);
			  if (rank == 0) {
				  averageWeights.DivideEquals(size);
				  VERBOSE(1, "After mixing: " << averageWeights << endl);

				  // normalise weights after averaging
				  averageWeights.L1Normalise();
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
				  totalWeights.DivideEquals(iterations);
			  else
				  totalWeights.DivideEquals(iterationsThisEpoch);

			  // average across processes
#ifdef MPI_ENABLE
			  mpi::reduce(world, totalWeights, averageTotalWeights, SCCPlus(), 0);
			  if (rank == 0) {
				  // average and normalise weights
				  averageTotalWeights.DivideEquals(size);
				  averageTotalWeights.L1Normalise();
			  }
#endif
#ifndef MPI_ENABLE
			  // or use weights from single process
			  averageTotalWeights = totalWeights;
#endif
			  if (!weightDumpStem.empty()) {
				  ostringstream filename;
				  filename << weightDumpStem << "_" << epoch;
				  if (weightDumpFrequency > 1) {
					  filename << "_" << weightEpochDump;
				  }

				  VERBOSE(1, "Dumping weights for epoch " << epoch << " to " << filename.str() << endl);
				  averageTotalWeights.Save(filename.str());
				  ++weightEpochDump;
			  }
		  }
	  }
  }
  
/*#ifdef MPI_ENABLE
			  mpi::finalize();
#endif*/

  cerr << "Average total weights: " << averageTotalWeights << endl;

  now = time(0); // get current time
  tm = localtime(&now); // get struct filled out
  cerr << "End date/time: " << tm->tm_mon+1 << "/" << tm->tm_mday << "/" << tm->tm_year + 1900
		    << ", " << tm->tm_hour << ":" << tm->tm_min << ":" << tm->tm_sec << endl;

  delete decoder;
  exit(0);
}

