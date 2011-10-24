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
#include <map>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#ifdef MPI_ENABLE
#include <boost/mpi.hpp>
namespace mpi = boost::mpi;
#endif

#include "Main.h"
#include "FeatureVector.h"
#include "StaticData.h"
#include "ChartTrellisPathList.h"
#include "ChartTrellisPath.h"
#include "ScoreComponentCollection.h"
#include "Decoder.h"
#include "Optimiser.h"
#include "Hildreth.h"
#include "ThreadPool.h"

using namespace Mira;
using namespace std;
using namespace Moses;
namespace po = boost::program_options;

int main(int argc, char** argv) {
	size_t rank = 0;
	size_t size = 1;
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
	string coreWeightFile;
	size_t epochs;
	string learner;
	bool shuffle;
	size_t mixingFrequency;
	size_t weightDumpFrequency;
	string weightDumpStem;
	float min_learning_rate;
	size_t scale_margin;
	size_t scale_update;
	size_t n;
	size_t batchSize;
	bool distinctNbest;
	bool onlyViolatedConstraints;
	bool accumulateWeights;
	float historySmoothing;
	bool scaleByInputLength;
	bool scaleByTargetLength;
	float scaleByX;
	float slack;
	float slack_step;
	float slack_min;
	bool averageWeights;
	bool weightConvergence;
	float learning_rate;
	float mira_learning_rate;
	float perceptron_learning_rate;
	bool logFeatureValues;
	size_t baseOfLog;
	string decoder_settings;
	float min_weight_change;
	float decrease_learning_rate;
	bool normaliseWeights;
	bool print_feature_values;
	bool historyOf1best;
	bool historyOfOracles;
	bool sentenceLevelBleu;
	float bleuScoreWeight;
	float bleuScoreWeight_hope;
	float margin_slack;
	float margin_slack_incr;
	bool perceptron_update;
	bool hope_fear;
	bool model_hope_fear;
	int hope_n;
	int fear_n;
	int threadcount;
	size_t adapt_after_epoch;
	size_t bleu_smoothing_scheme;
	po::options_description desc("Allowed options");
	desc.add_options()
		("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
		("adapt-after-epoch", po::value<size_t>(&adapt_after_epoch)->default_value(0), "Index of epoch after which adaptive parameters will be adapted")
		("average-weights", po::value<bool>(&averageWeights)->default_value(false), "Set decoder weights to average weights after each update")
		("base-of-log", po::value<size_t>(&baseOfLog)->default_value(10), "Base for log-ing feature values")
		("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
		("bleu-score-weight", po::value<float>(&bleuScoreWeight)->default_value(1.0), "Bleu score weight used in the decoder objective function (on top of the bleu objective weight)")
		("bleu-score-weight-hope", po::value<float>(&bleuScoreWeight_hope)->default_value(-1), "Bleu score weight used in the decoder objective function for hope translations")
		("bleu-smoothing-scheme", po::value<size_t>(&bleu_smoothing_scheme)->default_value(1), "Set a smoothing scheme for sentence-Bleu: +1 (1), +0.1, papineni (2) (default:1)")
		("config,f", po::value<string>(&mosesConfigFile), "Moses ini file")
		("core-weights", po::value<string>(&coreWeightFile), "Weight file containing the core weights (already tuned, have to be non-zero)")
		("decoder-settings", po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
		("decr-learning-rate", po::value<float>(&decrease_learning_rate)->default_value(0),"Decrease learning rate by the given value after every epoch")
		("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(true), "Use nbest list with distinct translations in inference step")
		("epochs,e", po::value<size_t>(&epochs)->default_value(10), "Number of epochs")
		("fear-n", po::value<int>(&fear_n)->default_value(-1), "Number of fear translations used")
		("help", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
		("history-of-1best", po::value<bool>(&historyOf1best)->default_value(false), "Use 1best translations to update the history")
		("history-of-oracles", po::value<bool>(&historyOfOracles)->default_value(false), "Use oracle translations to update the history")
		("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.7), "Adjust the factor for history smoothing")
		("hope-fear", po::value<bool>(&hope_fear)->default_value(true), "Use only hope and fear translations for optimization (not model)")
		("hope-n", po::value<int>(&hope_n)->default_value(-1), "Number of hope translations used")
		("input-file,i", po::value<string>(&inputFile), "Input file containing tokenised source")
		("learner,l", po::value<string>(&learner)->default_value("mira"), "Learning algorithm")
		("log-feature-values", po::value<bool>(&logFeatureValues)->default_value(false), "Take log of feature values according to the given base.")
		("margin-incr", po::value<float>(&margin_slack_incr)->default_value(0), "Increment margin slack after every epoch by this amount")
		("margin-slack", po::value<float>(&margin_slack)->default_value(0), "Slack when comparing left and right hand side of constraints")
		("min-learning-rate", po::value<float>(&min_learning_rate)->default_value(0), "Set a minimum learning rate")
		("min-weight-change", po::value<float>(&min_weight_change)->default_value(0.01), "Set minimum weight change for stopping criterion")
		("mira-learning-rate", po::value<float>(&mira_learning_rate)->default_value(1), "Learning rate for MIRA (fixed or flexible)")
		("mixing-frequency", po::value<size_t>(&mixingFrequency)->default_value(5), "How often per epoch to mix weights, when using mpi")
		("model-hope-fear", po::value<bool>(&model_hope_fear)->default_value(false), "Use model, hope and fear translations for optimization")
		("nbest,n", po::value<size_t>(&n)->default_value(1), "Number of translations in nbest list")
		("normalise", po::value<bool>(&normaliseWeights)->default_value(false), "Whether to normalise the updated weights before passing them to the decoder")
		("only-violated-constraints", po::value<bool>(&onlyViolatedConstraints)->default_value(false), "Add only violated constraints to the optimisation problem")
		("perceptron-learning-rate", po::value<float>(&perceptron_learning_rate)->default_value(0.01), "Perceptron learning rate")
		("print-feature-values", po::value<bool>(&print_feature_values)->default_value(false), "Print out feature values")
		("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training")
		("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale the BLEU score by (a history of) the input length")
		("scale-by-target-length", po::value<bool>(&scaleByTargetLength)->default_value(false), "Scale the BLEU score by (a history of) the candidate length")
		("scale-by-x", po::value<float>(&scaleByX)->default_value(1), "Scale the BLEU score by value x")
		("scale-margin", po::value<size_t>(&scale_margin)->default_value(0), "Scale the margin by the Bleu score of the oracle translation")
		("scale-update", po::value<size_t>(&scale_update)->default_value(0), "Scale the update by the Bleu score of the oracle translation")
		("sentence-level-bleu", po::value<bool>(&sentenceLevelBleu)->default_value(true), "Use a sentences level bleu scoring function")
		("shuffle", po::value<bool>(&shuffle)->default_value(false), "Shuffle input sentences before processing")
		("slack", po::value<float>(&slack)->default_value(0.01), "Use slack in optimizer")
		("slack-min", po::value<float>(&slack_min)->default_value(0.01), "Minimum slack used")
		("slack-step", po::value<float>(&slack_step)->default_value(0), "Increase slack from epoch to epoch by the value provided")
		("stop-weights", po::value<bool>(&weightConvergence)->default_value(true), "Stop when weights converge")
		("threads", po::value<int>(&threadcount)->default_value(1), "Number of threads used")
		("verbosity,v", po::value<int>(&verbosity)->default_value(0), "Verbosity level")
		("weight-dump-frequency", po::value<size_t>(&weightDumpFrequency)->default_value(1), "How often per epoch to dump weights, when using mpi")
		("weight-dump-stem", po::value<string>(&weightDumpStem)->default_value("weights"), "Stem of filename to use for dumping weights");

	po::options_description cmdline_options;
	cmdline_options.add(desc);
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv). options(cmdline_options).run(), vm);
	po::notify(vm);

	if (help) {
		std::cout << "Usage: " + string(argv[0])
		    + " -f mosesini-file -i input-file -r reference-file(s) [options]"
		    << std::endl;
		std::cout << desc << std::endl;
		return 0;
	}

  // create threadpool, if using multi-threaded decoding
  // note: multi-threading is done on sentence-level,
  // each thread translates one sentence
#ifdef WITH_THREADS
  if (threadcount < 1) {
    cerr << "Error: Need to specify a positive number of threads" << endl;
    exit(1);
  }
  ThreadPool pool(threadcount);
#else
  if (threadcount > 1) {
    cerr << "Error: Thread count of " << threadcount << " but moses not built with thread support" << endl;
    exit(1);
  }
#endif

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

	StrFloatMap coreWeightMap;
	if (!coreWeightFile.empty()) {
		if (!hope_fear) {
			cerr << "Error: using pre-tuned core weights is only implemented for hope/fear updates at the moment" << endl;
			return 1;
		}

		if (!loadWeights(coreWeightFile, coreWeightMap)) {
				cerr << "Error: Failed to load core weights from " << coreWeightFile << endl;
				return 1;
		}
		else {
			cerr << "Loaded core weights from " << coreWeightFile << ": " << endl;
			StrFloatMap::iterator p;
			for(p = coreWeightMap.begin(); p!=coreWeightMap.end(); ++p)
			{
				cerr << p->first << ": " << p->second << endl;
			}
		}
	}

	// load input and references
	vector<string> inputSentences;
	if (!loadSentences(inputFile, inputSentences)) {
		cerr << "Error: Failed to load input sentences from " << inputFile << endl;
		return 1;
	}

	vector<vector<string> > referenceSentences(referenceFiles.size());
	for (size_t i = 0; i < referenceFiles.size(); ++i) {
		if (!loadSentences(referenceFiles[i], referenceSentences[i])) {
			cerr << "Error: Failed to load reference sentences from "
			    << referenceFiles[i] << endl;
			return 1;
		}
		if (referenceSentences[i].size() != inputSentences.size()) {
			cerr << "Error: Input file length (" << inputSentences.size() << ") != ("
			    << referenceSentences[i].size() << ") length of reference file " << i
			    << endl;
			return 1;
		}
	}

	if (scaleByTargetLength) {
		scaleByInputLength = false;
	}

	// initialise Moses
	vector<string> decoder_params;
	boost::split(decoder_params, decoder_settings, boost::is_any_of("\t "));
	initMoses(mosesConfigFile, verbosity, decoder_params.size(), decoder_params);
	MosesDecoder* decoder = new MosesDecoder(scaleByInputLength, scaleByTargetLength, scaleByX, historySmoothing);
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

	// initialise optimizer
	Optimiser* optimiser = NULL;
	if (learner == "mira") {
		if (rank == 0) {
			cerr << "Optimising using Mira" << endl;
		}
		optimiser = new MiraOptimiser(onlyViolatedConstraints, slack, scale_margin, scale_update, margin_slack);
		learning_rate = mira_learning_rate;
		perceptron_update = false;
	} else if (learner == "perceptron") {
		if (rank == 0) {
			cerr << "Optimising using Perceptron" << endl;
		}
		optimiser = new Perceptron();
		learning_rate = perceptron_learning_rate;
		perceptron_update = true;
		model_hope_fear = false; // mira only
		hope_fear = false; // mira only
		n = 1;
		hope_n = 1;
		fear_n = 1;
	} else {
		cerr << "Error: Unknown optimiser: " << learner << endl;
		return 1;
	}

	// resolve parameter dependencies
	if (batchSize > 1 && perceptron_update) {
		batchSize = 1;
		cerr << "Info: Setting batch size to 1 for perceptron update" << endl;
	}
	if (hope_n == -1 && fear_n == -1) {
		hope_n = n;
		fear_n = n;
	}
	if (model_hope_fear && hope_fear) {
		hope_fear = false; // is true by default
	}
	if (learner == "mira" && !(hope_fear || model_hope_fear)) {
		cerr << "Error: Need to select an one of parameters --hope-fear/--model-hope-fear for mira update." << endl;
		return 1;
	}
	if (!sentenceLevelBleu) {
		if (!historyOf1best && !historyOfOracles) {
			historyOf1best = true;
		}
	}
	if (bleuScoreWeight_hope == -1) {
		bleuScoreWeight_hope = bleuScoreWeight;
	}

	// References are loaded by StaticData::LoadReferences() when the parameter "references" is specified in the ini file.
	// To be sure they are available, load explicitly here.
	decoder->loadReferenceSentences(referenceSentences);
	decoder->setBleuSmoothingScheme(bleu_smoothing_scheme);

#ifdef MPI_ENABLE
	mpi::broadcast(world, order, 0);
#endif

	// Create shards according to the number of processes used
	vector<size_t> shard;
	float shardSize = (float) (order.size()) / size;
	VERBOSE(1, "Shard size: " << shardSize << endl);
	size_t shardStart = (size_t) (shardSize * rank);
	size_t shardEnd = (size_t) (shardSize * (rank + 1));
	if (rank == size - 1)
		shardEnd = order.size();
	VERBOSE(1, "Rank: " << rank << " Shard start: " << shardStart << " Shard end: " << shardEnd << endl);
	shard.resize(shardSize);
	copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());

	// set core weights
	const vector<const ScoreProducer*> featureFunctions =
	    StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT).GetFeatureFunctions();
	ScoreComponentCollection initialWeights = decoder->getWeights();
	if (coreWeightMap.size() > 0) {
		StrFloatMap::iterator p;
		for(p = coreWeightMap.begin(); p!=coreWeightMap.end(); ++p)
		{
			initialWeights.Assign(p->first, p->second);
		}
	}
	decoder->setWeights(initialWeights);

	//Main loop:
	// print initial weights
	cerr << "Rank " << rank << ", initial weights: " << initialWeights << endl;
	ScoreComponentCollection cumulativeWeights; // collect weights per epoch to produce an average
	size_t numberOfUpdates = 0;
	size_t numberOfUpdatesThisEpoch = 0;

	time_t now;
	time(&now);
	cerr << "Rank " << rank << ", " << ctime(&now) << endl;

	ScoreComponentCollection mixedAverageWeights;
	ScoreComponentCollection mixedAverageWeightsPrevious;
	ScoreComponentCollection mixedAverageWeightsBeforePrevious;

	bool stop = false;
	int sumStillViolatedConstraints;
	float *sendbuf, *recvbuf;
	sendbuf = (float *) malloc(sizeof(float));
	recvbuf = (float *) malloc(sizeof(float));
	for (size_t epoch = 0; epoch < epochs && !stop; ++epoch) {
		// sum of violated constraints in an epoch
		sumStillViolatedConstraints = 0;

		numberOfUpdatesThisEpoch = 0;
		// Sum up weights over one epoch, final average uses weights from last epoch
		if (!accumulateWeights) {
			cumulativeWeights.ZeroAll();
		}

		// number of weight dumps this epoch
		size_t weightEpochDump = 0;

		size_t shardPosition = 0;
		vector<size_t>::const_iterator sid = shard.begin();
		while (sid != shard.end()) {
			// feature values for hypotheses i,j (matrix: batchSize x 3*n x featureValues)
			vector<vector<ScoreComponentCollection> > featureValues;
			vector<vector<float> > bleuScores;

			// variables for hope-fear/perceptron setting
			vector<vector<ScoreComponentCollection> > featureValuesHope;
			vector<vector<ScoreComponentCollection> > featureValuesFear;
			vector<vector<float> > bleuScoresHope;
			vector<vector<float> > bleuScoresFear;
			vector<vector<ScoreComponentCollection> > dummyFeatureValues;
			vector<vector<float> > dummyBleuScores;

			// get moses weights
			ScoreComponentCollection mosesWeights = decoder->getWeights();
			VERBOSE(1, "\nRank " << rank << ", epoch " << epoch << ", weights: " << mosesWeights << endl);

			// BATCHING: produce nbest lists for all input sentences in batch
			vector<float> oracleBleuScores;
			vector<vector<const Word*> > oracles;
			vector<vector<const Word*> > oneBests;
			vector<ScoreComponentCollection> oracleFeatureValues;
			vector<size_t> inputLengths;
			vector<size_t> ref_ids;
			size_t actualBatchSize = 0;

			vector<size_t>::const_iterator current_sid_start = sid;
			for (size_t batchPosition = 0; batchPosition < batchSize && sid
			    != shard.end(); ++batchPosition) {
				string& input = inputSentences[*sid];
				const vector<string>& refs = referenceSentences[*sid];
				cerr << "\nRank " << rank << ", epoch " << epoch << ", input sentence " << *sid << ": \"" << input << "\"" << " (batch pos " << batchPosition << ")" << endl;

				vector<ScoreComponentCollection> newFeatureValues;
				vector<float> newBleuScores;
				if (model_hope_fear) {
					featureValues.push_back(newFeatureValues);
					bleuScores.push_back(newBleuScores);
				}
				else {
					featureValuesHope.push_back(newFeatureValues);
					featureValuesFear.push_back(newFeatureValues);
					bleuScoresHope.push_back(newBleuScores);
					bleuScoresFear.push_back(newBleuScores);
					if (historyOf1best) {
						dummyFeatureValues.push_back(newFeatureValues);
						dummyBleuScores.push_back(newBleuScores);
					}
				}

				size_t reference_length = decoder->getReferenceLength(*sid);
				if (hope_fear || perceptron_update) {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << hope_n << "best hope translations" << endl;
					vector<const Word*> oracle = decoder->getNBest(input, *sid, hope_n, 1.0, bleuScoreWeight_hope,
							featureValuesHope[batchPosition], bleuScoresHope[batchPosition], true,
							distinctNbest, rank, epoch);
					// needed for history
					inputLengths.push_back(decoder->getCurrentInputLength());
					ref_ids.push_back(*sid);
					decoder->cleanup();
					oracles.push_back(oracle);
					float hope_length_ratio = (float)oracle.size()/reference_length;
					cerr << ", l-ratio hope: " << hope_length_ratio << endl;

					if (historyOf1best) {
						// MODEL (for updating the history only, using dummy vectors)
						cerr << "Rank " << rank << ", epoch " << epoch << ", 1best wrt model score (for history)" << endl;
						vector<const Word*> bestModel = decoder->getNBest(input, *sid, 1, 0.0, bleuScoreWeight,
								dummyFeatureValues[batchPosition], dummyBleuScores[batchPosition], true,
								distinctNbest, rank, epoch);
						decoder->cleanup();
						oneBests.push_back(bestModel);
						float model_length_ratio = (float)bestModel.size()/reference_length;
						cerr << ", l-ratio model: " << model_length_ratio << endl;
					}

					// FEAR
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << fear_n << "best fear translations" << endl;
					vector<const Word*> fear = decoder->getNBest(input, *sid, fear_n, -1.0, bleuScoreWeight,
							featureValuesFear[batchPosition], bleuScoresFear[batchPosition], true,
							distinctNbest, rank, epoch);
					decoder->cleanup();
					float fear_length_ratio = (float)fear.size()/reference_length;
					cerr << ", l-ratio fear: " << fear_length_ratio << endl;
					for (size_t i = 0; i < fear.size(); ++i) {
						delete fear[i];
					}

					cerr << "Rank " << rank << ", epoch " << epoch << ", abs-length hope-fear: " << abs((int)oracle.size() - (int)fear.size()) << ", BLEU hope-fear: " << bleuScoresHope[batchPosition][0] - bleuScoresFear[batchPosition][0] << endl;
				}
				else {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best hope translations" << endl;
					size_t oraclePos = featureValues[batchPosition].size();
					vector<const Word*> oracle = decoder->getNBest(input, *sid, n, 1.0, bleuScoreWeight_hope,
							featureValues[batchPosition], bleuScores[batchPosition], true,
							distinctNbest, rank, epoch);
					// needed for history
					inputLengths.push_back(decoder->getCurrentInputLength());
					ref_ids.push_back(*sid);
					decoder->cleanup();
					oracles.push_back(oracle);
					float hope_length_ratio = (float)oracle.size()/reference_length;
					cerr << ", l-ratio hope: " << hope_length_ratio << endl;

					oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
					oracleBleuScores.push_back(bleuScores[batchPosition][oraclePos]);

					// MODEL
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best wrt model score" << endl;
					vector<const Word*> bestModel = decoder->getNBest(input, *sid, n, 0.0, bleuScoreWeight,
							featureValues[batchPosition], bleuScores[batchPosition], true,
							distinctNbest, rank, epoch);
					decoder->cleanup();
					oneBests.push_back(bestModel);
					float model_length_ratio = (float)bestModel.size()/reference_length;
					cerr << ", l-ratio model: " << model_length_ratio << endl;

					// FEAR
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best fear translations" << endl;
					size_t fearPos = featureValues[batchPosition].size();
					vector<const Word*> fear = decoder->getNBest(input, *sid, n, -1.0, bleuScoreWeight,
							featureValues[batchPosition], bleuScores[batchPosition], true,
							distinctNbest, rank, epoch);
					decoder->cleanup();
					float fear_length_ratio = (float)fear.size()/reference_length;
					cerr << ", l-ratio fear: " << fear_length_ratio << endl;
					for (size_t i = 0; i < fear.size(); ++i) {
						delete fear[i];
					}
				}

				// next input sentence
				++sid;
				++actualBatchSize;
				++shardPosition;
			} // end of batch loop

			vector<vector<float> > losses(actualBatchSize);
			if (model_hope_fear) {
				// Set loss for each sentence as BLEU(oracle) - BLEU(hypothesis)
				for (size_t batchPosition = 0; batchPosition < actualBatchSize; ++batchPosition) {
					for (size_t j = 0; j < bleuScores[batchPosition].size(); ++j) {
						losses[batchPosition].push_back(oracleBleuScores[batchPosition] - bleuScores[batchPosition][j]);
					}
				}
			}

			// set weight for bleu feature to 0
			mosesWeights.Assign(featureFunctions.back(), 0);

			// take logs of feature values
			if (logFeatureValues) {
				takeLogs(featureValuesHope, baseOfLog);
				takeLogs(featureValuesFear, baseOfLog);
				takeLogs(featureValues, baseOfLog);
				for (size_t i = 0; i < oracleFeatureValues.size(); ++i) {
					oracleFeatureValues[i].ApplyLog(baseOfLog);
				}
			}

			// print out the feature values
			if (print_feature_values) {
				cerr << "\nRank " << rank << ", epoch " << epoch << ", feature values: " << endl;
				if (model_hope_fear) printFeatureValues(featureValues);
				else {
					cerr << "hope: " << endl;
					printFeatureValues(featureValuesHope);
					cerr << "fear: " << endl;
					printFeatureValues(featureValuesFear);
				}
			}

			// set core features to 0 to avoid updating the feature weights
			if (coreWeightMap.size() > 0) {
				ignoreCoreFeatures(featureValues, coreWeightMap);
				ignoreCoreFeatures(featureValuesHope, coreWeightMap);
				ignoreCoreFeatures(featureValuesFear, coreWeightMap);
			}

			// Run optimiser on batch:
			VERBOSE(1, "\nRank " << rank << ", epoch " << epoch << ", run optimiser:" << endl);
			size_t update_status;
			if (perceptron_update) {
				vector<vector<float> > dummy1;
				vector<size_t> dummy2;
				update_status = optimiser->updateWeightsHopeFear(mosesWeights,
						featureValuesHope, featureValuesFear, dummy1, dummy1, dummy2,
						learning_rate, rank, epoch);
			}
			else if (hope_fear) {
				update_status = optimiser->updateWeightsHopeFear(mosesWeights,
						featureValuesHope, featureValuesFear, bleuScoresHope, bleuScoresFear, ref_ids,
						learning_rate, rank, epoch);
			}
			else {
				// model_hope_fear
				update_status = ((MiraOptimiser*) optimiser)->updateWeights(mosesWeights,
						featureValues, losses, bleuScores, oracleFeatureValues, oracleBleuScores, ref_ids,
						learning_rate, rank, epoch);
			}

			sumStillViolatedConstraints += update_status;

			// pass new weights to decoder
			if (normaliseWeights) {
				mosesWeights.L1Normalise();
			}

			cumulativeWeights.PlusEquals(mosesWeights);
			++numberOfUpdates;
			++numberOfUpdatesThisEpoch;
			if (averageWeights) {
				ScoreComponentCollection averageWeights(cumulativeWeights);
				if (accumulateWeights) {
					averageWeights.DivideEquals(numberOfUpdates);
				} else {
					averageWeights.DivideEquals(numberOfUpdatesThisEpoch);
				}

				mosesWeights = averageWeights;
			}

			// set new Moses weights
			decoder->setWeights(mosesWeights);

			// update history (for approximate document Bleu)
			if (sentenceLevelBleu) {
				if (verbosity > 0) {
					for (size_t i = 0; i < oracles.size(); ++i) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", oracle length: " << oracles[i].size() << " ";
						decoder->printReferenceLength(ref_ids);
					}
				}
			}
			else {
				if (historyOf1best) {
					for (size_t i = 0; i < oneBests.size(); ++i) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", update history with 1best length: " << oneBests[i].size() << " ";
					}
					decoder->updateHistory(oneBests, inputLengths, ref_ids, rank, epoch);
				}
				else {
					for (size_t i = 0; i < oracles.size(); ++i) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", update history with oracle length: " << oracles[i].size() << " ";
					}
					decoder->updateHistory(oracles, inputLengths, ref_ids, rank, epoch);
				}
			}

			// clean up oracle and 1best translations after updating history
			deleteTranslations(oracles);
			deleteTranslations(oneBests);

			size_t mixing_base = mixingFrequency == 0 ? 0 : shard.size() / mixingFrequency;
			size_t dumping_base = weightDumpFrequency ==0 ? 0 : shard.size() / weightDumpFrequency;
			// mix weights?
			if (evaluateModulo(shardPosition, mixing_base, actualBatchSize)) {
#ifdef MPI_ENABLE
				ScoreComponentCollection mixedWeights;
				cerr << "\nRank " << rank << ", before mixing: " << mosesWeights << endl;

				// collect all weights in mixedWeights and divide by number of processes
				mpi::reduce(world, mosesWeights, mixedWeights, SCCPlus(), 0);
				if (rank == 0) {
					// divide by number of processes
					mixedWeights.DivideEquals(size);

					// normalise weights after averaging
					if (normaliseWeights) {
						mixedWeights.L1Normalise();
						cerr << "Mixed weights (normalised): " << mixedWeights << endl;
					}
					else {
						cerr << "Mixed weights: " << mixedWeights << endl;
					}
				}

				// broadcast average weights from process 0
				mpi::broadcast(world, mixedWeights, 0);
				decoder->setWeights(mixedWeights);
				mosesWeights = mixedWeights;
#endif
#ifndef MPI_ENABLE
				cerr << "\nRank " << rank << ", no mixing, weights: " << mosesWeights << endl;
#endif
			} // end mixing

			// Dump weights?
			if (evaluateModulo(shardPosition, dumping_base, actualBatchSize)) {
				ScoreComponentCollection tmpAverageWeights(cumulativeWeights);
				if (accumulateWeights) {
					tmpAverageWeights.DivideEquals(numberOfUpdates);
				} else {
					tmpAverageWeights.DivideEquals(numberOfUpdatesThisEpoch);
				}

#ifdef MPI_ENABLE
				// average across processes
				mpi::reduce(world, tmpAverageWeights, mixedAverageWeights, SCCPlus(), 0);
#endif
#ifndef MPI_ENABLE
				mixedAverageWeights = tmpAverageWeights;
#endif
				if (rank == 0 && !weightDumpStem.empty()) {
					// divide by number of processes
					mixedAverageWeights.DivideEquals(size);

					// normalise weights after averaging
					if (normaliseWeights) {
						mixedAverageWeights.L1Normalise();
					}

					// dump final average weights
					ostringstream filename;
					if (epoch < 10) {
							filename << weightDumpStem << "_0" << epoch;
					} else {
						filename << weightDumpStem << "_" << epoch;
					}

					if (weightDumpFrequency > 1) {
						filename << "_" << weightEpochDump;
					}

					if (accumulateWeights) {
						cerr << "\nMixed average weights (cumulative) during epoch "	<< epoch << ": " << mixedAverageWeights << endl;
					} else {
						cerr << "\nMixed average weights during epoch " << epoch << ": " << mixedAverageWeights << endl;
					}

					cerr << "Dumping mixed average weights during epoch " << epoch << " to " << filename.str() << endl << endl;
					mixedAverageWeights.Save(filename.str());
					++weightEpochDump;
				}
			}// end dumping
		} // end of shard loop, end of this epoch

		if (verbosity > 0) {
			cerr << "Bleu feature history after epoch " <<  epoch << endl;
			decoder->printBleuFeatureHistory(cerr);
		}
		cerr << "Rank " << rank << ", epoch " << epoch << ", sum of violated constraints: " << sumStillViolatedConstraints << endl;

		// Check whether there were any weight updates during this epoch
		size_t sumUpdates;
		size_t *sendbuf_uint, *recvbuf_uint;
		sendbuf_uint = (size_t *) malloc(sizeof(size_t));
		recvbuf_uint = (size_t *) malloc(sizeof(size_t));
#ifdef MPI_ENABLE
		//mpi::reduce(world, numberOfUpdatesThisEpoch, sumUpdates, MPI_SUM, 0);
		sendbuf_uint[0] = numberOfUpdatesThisEpoch;
		recvbuf_uint[0] = 0;
		MPI_Reduce(sendbuf_uint, recvbuf_uint, 1, MPI_UNSIGNED, MPI_SUM, 0, world);
		sumUpdates = recvbuf_uint[0];
#endif
#ifndef MPI_ENABLE
		sumUpdates = numberOfUpdatesThisEpoch;
#endif
		if (rank == 0 && sumUpdates == 0) {
		  cerr << "\nNo weight updates during this epoch.. stopping." << endl;
		  stop = true;
#ifdef MPI_ENABLE
		  mpi::broadcast(world, stop, 0);
#endif
		}

		if (!stop) {
			// Test if weights have converged
			if (weightConvergence) {
				bool reached = true;
				if (rank == 0 && (epoch >= 2)) {
					ScoreComponentCollection firstDiff(mixedAverageWeights);
					firstDiff.MinusEquals(mixedAverageWeightsPrevious);
					VERBOSE(1, "Average weight changes since previous epoch: " << firstDiff << endl);
					ScoreComponentCollection secondDiff(mixedAverageWeights);
					secondDiff.MinusEquals(mixedAverageWeightsBeforePrevious);
					VERBOSE(1, "Average weight changes since before previous epoch: " << secondDiff << endl << endl);

					// check whether stopping criterion has been reached
					// (both difference vectors must have all weight changes smaller than min_weight_change)
					FVector changes1 = firstDiff.GetScoresVector();
					FVector changes2 = secondDiff.GetScoresVector();
					FVector::const_iterator iterator1 = changes1.cbegin();
					FVector::const_iterator iterator2 = changes2.cbegin();
					while (iterator1 != changes1.cend()) {
						if (abs((*iterator1).second) >= min_weight_change || abs(
								(*iterator2).second) >= min_weight_change) {
							reached = false;
							break;
						}

						++iterator1;
						++iterator2;
					}

					if (reached) {
						// stop MIRA
						stop = true;
						cerr << "\nWeights have converged after epoch " << epoch << ".. stopping MIRA." << endl;
						ScoreComponentCollection dummy;
						ostringstream endfilename;
						endfilename << "stopping";
						dummy.Save(endfilename.str());
					}
				}

				mixedAverageWeightsBeforePrevious = mixedAverageWeightsPrevious;
				mixedAverageWeightsPrevious = mixedAverageWeights;
#ifdef MPI_ENABLE
				mpi::broadcast(world, stop, 0);
#endif
			} //end if (weightConvergence)

			// adjust flexible parameters
			if (!stop && epoch >= adapt_after_epoch) {
				// if using flexible slack, decrease slack parameter for next epoch
				if (slack_step > 0) {
					if (slack - slack_step >= slack_min) {
						if (typeid(*optimiser) == typeid(MiraOptimiser)) {
							slack -= slack_step;
							VERBOSE(1, "Change slack to: " << slack << endl);
							((MiraOptimiser*) optimiser)->setSlack(slack);
						}
					}
				}

				// if using flexible margin slack, decrease margin slack parameter for next epoch
				if (margin_slack_incr > 0.0001) {
					if (typeid(*optimiser) == typeid(MiraOptimiser)) {
						margin_slack += margin_slack_incr;
						VERBOSE(1, "Change margin slack to: " << margin_slack << endl);
						((MiraOptimiser*) optimiser)->setMarginSlack(margin_slack);
					}
				}

				// change learning rate
				if ((decrease_learning_rate > 0) && (learning_rate - decrease_learning_rate >= min_learning_rate)) {
					learning_rate -= decrease_learning_rate;
					if (learning_rate <= 0.0001) {
						learning_rate = 0;
						stop = true;
#ifdef MPI_ENABLE
						mpi::broadcast(world, stop, 0);
#endif
					}
					VERBOSE(1, "Change learning rate to " << learning_rate << endl);
				}
			}
		}
	} // end of epoch loop

#ifdef MPI_ENABLE
	MPI_Finalize();
#endif

	time(&now);
	cerr << "Rank " << rank << ", " << ctime(&now);

	delete decoder;
	exit(0);
}

bool loadSentences(const string& filename, vector<string>& sentences) {
	ifstream in(filename.c_str());
	if (!in)
		return false;
	string line;
	while (getline(in, line)) {
		sentences.push_back(line);
	}
	return true;
}

bool loadWeights(const string& filename, StrFloatMap& coreWeightMap) {
	ifstream in(filename.c_str());
	if (!in)
		return false;
	string line;
	while (getline(in, line)) {
		// split weight name from value
		vector<string> split_line;
		boost::split(split_line, line, boost::is_any_of(" "));

		float weight;
		if(!from_string<float>(weight, split_line[1], std::dec))
		{
			cerr << "from_string failed" << endl;
			return false;
		}
		coreWeightMap.insert(StrFloatPair(split_line[0], weight));
	}
	return true;
}

bool evaluateModulo(size_t shard_position, size_t mix_or_dump_base, size_t actual_batch_size) {
	if (mix_or_dump_base == 0) return 0;
	if (actual_batch_size > 1) {
		bool mix_or_dump = false;
		size_t numberSubtracts = actual_batch_size;
		do {
			if (shard_position % mix_or_dump_base == 0) {
				mix_or_dump = true;
				break;
			}
			--shard_position;
			--numberSubtracts;
		} while (numberSubtracts > 0);
		return mix_or_dump;
	}
	else {
		return ((shard_position % mix_or_dump_base) == 0);
	}
}

void printFeatureValues(vector<vector<ScoreComponentCollection> > &featureValues) {
	for (size_t i = 0; i < featureValues.size(); ++i) {
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			cerr << featureValues[i][j] << endl;
		}
	}
	cerr << endl;
}

void ignoreCoreFeatures(vector<vector<ScoreComponentCollection> > &featureValues, StrFloatMap &coreWeightMap) {
	for (size_t i = 0; i < featureValues.size(); ++i) {
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			// set all core features to 0
			StrFloatMap::iterator p;
			for(p = coreWeightMap.begin(); p!=coreWeightMap.end(); ++p)
			{
				featureValues[i][j].Assign(p->first, 0);
			}
		}
	}
}

void takeLogs(vector<vector<ScoreComponentCollection> > &featureValues, size_t base) {
	for (size_t i = 0; i < featureValues.size(); ++i) {
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			featureValues[i][j].ApplyLog(base);
		}
	}
}

void deleteTranslations(vector<vector<const Word*> > &translations) {
	for (size_t i = 0; i < translations.size(); ++i) {
		for (size_t j = 0; j < translations[i].size(); ++j) {
			delete translations[i][j];
		}
	}
}
