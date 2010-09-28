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
  bool shuffle = true;
  size_t mixFrequency;
  size_t weightDumpFrequency;
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
        ("weight-dump-frequency", po::value<size_t>(&weightDumpFrequency)->default_value(1), "How often per epoch to dump weights");

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

  //FIXME: Make these configurable
  float miraLowerBound = 0;
  float miraUpperBound = 1;

  Optimiser* optimiser = NULL;
  if (learner == "mira") {
    cerr << "Optimising using Mira" << endl;
    optimiser = new MiraOptimiser(miraLowerBound, miraUpperBound);
  } else if (learner == "perceptron") {
    cerr << "Optimising using Perceptron" << endl;
    optimiser = new Perceptron();
  } else {
    cerr << "Error: Unknown optimiser: " << learner << endl;
  }


  //load input and references 
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
  //initialise moses
  initMoses(mosesConfigFile, verbosity);//, argc, argv);
  MosesDecoder* decoder = new MosesDecoder(referenceSentences) ;

  //Optionally shuffle the sentences
  vector<size_t> order;
  if (rank == 0) {
    for (size_t i = 0; i < inputSentences.size(); ++i) {
      order.push_back(i);
    }
    if (shuffle) {
      RandomIndex rindex;
      random_shuffle(order.begin(), order.end(), rindex);
    }
  }

#ifdef MPI_ENABLE
  mpi::broadcast(world,order,0);
#endif

  //Create the shards
  vector<size_t> shard;
  float shardSize = (float)(order.size()) / size;
  VERBOSE(1, "Shard size: " << shardSize << endl);
  size_t shardStart = (size_t)(shardSize * rank);
  size_t shardEnd = (size_t)(shardSize * (rank+1));
  if (rank == size-1) shardEnd = order.size();
  VERBOSE(1,"Rank: " << rank << " Shard start: " << shardStart << " Shard end: " << shardEnd << endl);
  shard.resize(shardSize);
  copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());

  

  //Main loop:
  ScoreComponentCollection cumulativeWeights;
  size_t modelHypoCount = 10;
  size_t hopeHypoCount = 10;
  size_t fearHypoCount = 10;
  size_t iterations = 0;
  
	
  for (size_t epoch = 0; epoch < epochs; ++epoch) {
    //TODO: batching
    size_t shardPosition = 0;
    for (vector<size_t>::const_iterator sid = shard.begin();
       sid != shard.end(); ++sid) {
      const string& input = inputSentences[*sid];
      const vector<string>& refs = referenceSentences[*sid];

      vector<vector<ScoreComponentCollection > > allScores(1);
      vector<vector<float> > allLosses(1);


			// MODEL
      decoder->getNBest(input,
                        *sid,
                        modelHypoCount,
                        0.0,
                        1.0,
                        allScores[0],
                        allLosses[0]);


			// HOPE
      size_t oraclePos = allScores.size();
      vector<const Word*> oracle =
         decoder->getNBest(input,
											 *sid,
												modelHypoCount,
                        1.0,
                        1.0,
                        allScores[0],
                        allLosses[0]);

      ScoreComponentCollection oracleScores = allScores[0][oraclePos];
      float oracleLoss = allLosses[0][oraclePos];
			
			// FEAR
      decoder->getNBest(input,
                        *sid,
                        modelHypoCount,
                        -1.0,
                        1.0,
                        allScores[0],
                        allLosses[0]);

      //set loss for each sentence as oracleloss - rawsentenceloss
      for (size_t i = 0; i < allScores.size(); ++i) {
        for (size_t j = 0; j < allScores[i].size(); ++j) {
          allLosses[i][j] = oracleLoss - allLosses[i][j];
        }
      }

			
      //run optimiser
		  ScoreComponentCollection mosesWeights = decoder->getWeights();	
			optimiser->updateWeights(mosesWeights
															, allScores
															, allLosses
															, oracleScores);
			
      //update moses weights
      mosesWeights.L1Normalise();
      decoder->setWeights(mosesWeights);
  
      //history (for approx doc bleu)
      decoder->updateHistory(oracle);
      cumulativeWeights.PlusEquals(mosesWeights);
			decoder->cleanup();

      ++shardPosition;
      ++iterations;

      //mix weights?
#ifdef MPI_ENABLE
      if (shardPosition % (shard.size() / mixFrequency) == 0) {
        ScoreComponentCollection averageWeights;
        VERBOSE(1, "Rank: " << rank << "Before mixing: " << mosesWeights << endl);
        mpi::reduce(world,mosesWeights,averageWeights,SCCPlus(),0);
        if (rank == 0) {
          averageWeights.MultiplyEquals(1.0f/size);
          VERBOSE(1, "After mixing: " << averageWeights << endl);
        }
        mpi::broadcast(world,averageWeights,0);
        decoder->setWeights(averageWeights);
      }
#endif

      //dump weights?
      if (shardPosition % (shard.size() / weightDumpFrequency) == 0) {
        ScoreComponentCollection totalWeights(cumulativeWeights);
#ifdef MPI_ENABLE
      //average across processes
      mpi::reduce(world,cumulativeWeights,totalWeights,SCCPlus(),0);
#endif
        if (rank == 0) {
          cout << "WEIGHTS " << iterations << " ";
          totalWeights.L1Normalise();
          cout << totalWeights << endl;
        }
      }
    }
  }
  


  exit(0);
}

