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
  float clipping;
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
	    ("clipping", po::value<float>(&clipping)->default_value(0.01f), "Set a clipping threshold to regularise updates");

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

  // initialise moses
  initMoses(mosesConfigFile, verbosity);//, argc, argv);
  MosesDecoder* decoder = new MosesDecoder(referenceSentences) ;
  ScoreComponentCollection startWeights = decoder->getWeights();

  // print feature function and weights
  // TODO: scaling of feature functions
  const vector<const ScoreProducer*> featureFunctions = StaticData::Instance().GetTranslationSystem (TranslationSystem::DEFAULT).GetFeatureFunctions();
  for (size_t i = 0; i < featureFunctions.size(); ++i) {
	  cerr << "Feature functions: " << featureFunctions[i]->GetScoreProducerDescription() << ": " << featureFunctions[i]->GetNumScoreComponents() << endl;
	  vector< float> weights = startWeights.GetScoresForProducer(featureFunctions[i]);
	  cerr << "weights: ";
	  for (size_t j = 0; j < weights.size(); ++j) {
		  cerr << weights[j] << " ";
	  }
	  cerr << endl;
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
  size_t n = 10;								// size of n-best lists
  if (learner == "mira") {
    cerr << "Optimising using Mira" << endl;
    optimiser = new MiraOptimiser(n, hildreth, clipping);
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
  size_t iterations = 0;

  time_t now = time(0); // get current time
  struct tm* tm = localtime(&now); // get struct filled out
  cerr << "Start date/time: " << tm->tm_mon+1 << "/" << tm->tm_mday << "/" << tm->tm_year + 1900
		    << ", " << tm->tm_hour << ":" << tm->tm_min << ":" << tm->tm_sec << endl;
  
  // the result of accumulating and averaging weights over one epoch and possibly several processes
  ScoreComponentCollection averageTotalWeights;

  // TODO: stop MIRA when score on dev or tuning set does not improve further?
  for (size_t epoch = 0; epoch < epochs; ++epoch) {
	  cerr << "\nEpoch " << epoch << endl;
	  // Sum up weights over one epoch, final average uses weights from last epoch
	  cumulativeWeights.ZeroAll();

	  //number of weight dumps this epoch
	  size_t weightEpochDump = 0;

	  // compute sum in objective function after each epoch
	  float maxSum = 0.0;

	  //TODO: batching
	  size_t batchSize = 1;
	  size_t batch = 0;
	  size_t shardPosition = 0;
	  for (vector<size_t>::const_iterator sid = shard.begin(); sid != shard.end(); ++sid) {
		  const string& input = inputSentences[*sid];
		  const vector<string>& refs = referenceSentences[*sid];
		  cerr << "\nInput sentence " << *sid << ": \"" << input << "\"" << std::endl;

		  // feature values for hypotheses i,j (matrix: batchSize x 3*n x featureValues)
		  vector<vector<ScoreComponentCollection > > featureValues(batchSize);
		  vector<vector<float> > bleuScores(batchSize);

		  // MODEL
		  cerr << "Run decoder to get nbest wrt model score" << std::endl;
		  decoder->getNBest(input,
                        *sid,
                        n,
                        0.0,
                        1.0,
                        featureValues[batch],
                        bleuScores[batch],
                        false);
		  decoder->cleanup();

		  // HOPE
		  cerr << "Run decoder to get nbest hope translations" << std::endl;
		  size_t oraclePos = featureValues[batch].size();
		  vector<const Word*> oracle = decoder->getNBest(input,
						*sid,
						n,
                        1.0,
                        1.0,
                        featureValues[batch],
                        bleuScores[batch],
                        true);
		  decoder->cleanup();

		  ScoreComponentCollection oracleFeatureValues = featureValues[batch][oraclePos];
		  float oracleBleuScore = bleuScores[batch][oraclePos];

		  // FEAR
		  cerr << "Run decoder to get nbest fear translations" << std::endl;
		  decoder->getNBest(input,
                        *sid,
                        n,
                        -1.0,
                        1.0,
                        featureValues[batch],
                        bleuScores[batch],
                        false);
		  decoder->cleanup();

	      // Set loss for each sentence as BLEU(oracle) - BLEU(hypothesis)
	      vector< vector<float> > losses(batchSize);
	      for (size_t i = 0; i < batchSize; ++i) {
	    	  for (size_t j = 0; j < bleuScores[i].size(); ++j) {
	    		  losses[i].push_back(oracleBleuScore - bleuScores[i][j]);
	    		  //cout << "loss[" << i << "," << j << "]" << endl;
	    	  }
	      }

	      // get weight vector and set weight for bleu feature to 0
	      ScoreComponentCollection mosesWeights = decoder->getWeights();
	      const vector<const ScoreProducer*> featureFunctions = StaticData::Instance().GetTranslationSystem (TranslationSystem::DEFAULT).GetFeatureFunctions();
	      mosesWeights.Assign(featureFunctions.back(), 0);
	      ScoreComponentCollection oldWeights(mosesWeights);
			
		  //run optimiser
	      cerr << "Run optimiser.." << endl;
	      optimiser->updateWeights(mosesWeights, featureValues, losses, oracleFeatureValues);

		  //update moses weights
	      mosesWeights.L1Normalise();
		  decoder->setWeights(mosesWeights);
  
		  //history (for approx doc bleu)
		  decoder->updateHistory(oracle);

		  cumulativeWeights.PlusEquals(mosesWeights);

		  // sanity check: compare margin created by old weights against new weights
		  float lossMinusMargin_old = 0;
		  float lossMinusMargin_new = 0;
	      for (size_t j = 0; j < featureValues.size(); ++j) {
	    	  ScoreComponentCollection featureDiff(oracleFeatureValues);
	    	  featureDiff.MinusEquals(featureValues[batch][j]);

	    	  // old weights
	    	  float margin = featureDiff.InnerProduct(oldWeights);
	    	  lossMinusMargin_old += (losses[batch][j] - margin);

	    	  // new weights
	    	  margin = losses[batch][j] - featureDiff.InnerProduct(mosesWeights);
	    	  lossMinusMargin_new += margin;
	      }

	      cerr << "\nSummed (loss - margin) with old weights: " << lossMinusMargin_old << endl;
	      cerr << "Summed (loss - margin) with new weights: " << lossMinusMargin_new << endl;
	      if (lossMinusMargin_new > lossMinusMargin_old) {
	    	  cerr << "Worsening: " << lossMinusMargin_new - lossMinusMargin_old << endl;
	      }

		  ++shardPosition;
		  ++iterations;

		  //mix weights?
#ifdef MPI_ENABLE
		  if (shardPosition % (shard.size() / mixFrequency) == 0) {
			  ScoreComponentCollection averageWeights;
			  VERBOSE(1, "\nRank: " << rank << " Before mixing: " << mosesWeights << endl);

			  // collect all weights in averageWeights and divide by number of processes
			  mpi::reduce(world, mosesWeights, averageWeights, SCCPlus(), 0);
			  if (rank == 0) {
				  averageWeights.DivideEquals(size);
				  //VERBOSE(1, "After mixing: " << averageWeights << endl);
			  }

			  // broadcast average weights from process 0
			  mpi::broadcast(world, averageWeights, 0);
			  decoder->setWeights(averageWeights);
		  }
#endif

		  //dump weights?
		  if (shardPosition % (shard.size() / weightDumpFrequency) == 0) {
			  // compute average weights per process over iterations
			  ScoreComponentCollection totalWeights(cumulativeWeights);
			  totalWeights.DivideEquals(iterations);

			  // average across processes
#ifdef MPI_ENABLE
			  mpi::reduce(world, totalWeights, averageTotalWeights, SCCPlus(), 0);
#endif
			  if (rank == 0 && !weightDumpStem.empty()) {
				  averageTotalWeights.DivideEquals(size);

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

		  for (size_t i = 0; i < oracle.size(); ++i) {
			  delete oracle[i];
		  }
	  }
  }
  
  cerr << "Final new weights: " << averageTotalWeights << endl;

  now = time(0); // get current time
  tm = localtime(&now); // get struct filled out
  cerr << "End date/time: " << tm->tm_mon+1 << "/" << tm->tm_mday << "/" << tm->tm_year + 1900
		    << ", " << tm->tm_hour << ":" << tm->tm_min << ":" << tm->tm_sec << endl;

  delete decoder;
  exit(0);
}

