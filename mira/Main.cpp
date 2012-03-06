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
	bool scaleByReferenceLength;
	bool scaleByTargetLengthLinear;
	bool scaleByTargetLengthTrend;
	bool scaleByAvgLength;
	float scaleByX;
	float slack, dummy;
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
	bool normaliseWeights, normaliseMargin;
	bool print_feature_values;
	bool historyOf1best;
	bool historyOfOracles;
	bool sentenceLevelBleu;
	float bleuScoreWeight, bleuScoreWeight_hope, bleuScoreWeight_fear;
	float margin_slack;
	float margin_slack_incr;
	bool perceptron_update;
	bool hope_fear, hope_fear_rank, hope_model;
	bool model_hope_fear, rank_only;
	int hope_n, fear_n, rank_n;
	int threadcount;
	size_t adapt_after_epoch;
	size_t bleu_smoothing_scheme;
	float max_length_dev_all;
	float max_length_dev_hypos;
	float max_length_dev_hope_ref;
	float max_length_dev_fear_ref;
	float relax_BP;
	bool delayUpdates;
	float min_oracle_bleu;
	float minBleuRatio, maxBleuRatio;
	bool boost;
	bool decode_hope, decode_fear, decode_model;
	string decode_filename;
	size_t update_scheme;
	po::options_description desc("Allowed options");
	desc.add_options()
		("slack", po::value<float>(&slack)->default_value(0.01), "Use slack in optimiser")
		("dummy", po::value<float>(&dummy)->default_value(-1), "Dummy variable for slack")
		("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
		("adapt-after-epoch", po::value<size_t>(&adapt_after_epoch)->default_value(0), "Index of epoch after which adaptive parameters will be adapted")
		("average-weights", po::value<bool>(&averageWeights)->default_value(false), "Set decoder weights to average weights after each update")
		("base-of-log", po::value<size_t>(&baseOfLog)->default_value(10), "Base for taking logs of feature values")
		("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
		("bleu-score-weight", po::value<float>(&bleuScoreWeight)->default_value(1.0), "Bleu score weight used in the decoder objective function (on top of the Bleu objective weight)")
		("bleu-score-weight-hope", po::value<float>(&bleuScoreWeight_hope)->default_value(-1), "Bleu score weight used in the decoder objective function for hope translations")
	  ("bleu-score-weight-fear", po::value<float>(&bleuScoreWeight_fear)->default_value(-1), "Bleu score weight used in the decoder objective function for fear translations")
		("bleu-smoothing-scheme", po::value<size_t>(&bleu_smoothing_scheme)->default_value(1), "Set a smoothing scheme for sentence-Bleu: +1 (1), +0.1 (2), papineni (3) (default:1)")
		("boost", po::value<bool>(&boost)->default_value(false), "Apply boosting factor to updates on misranked candidates")
		("config,f", po::value<string>(&mosesConfigFile), "Moses ini-file")
		("core-weights", po::value<string>(&coreWeightFile), "Weight file containing the core weights (already tuned, have to be non-zero)")
		("decode-hope", po::value<bool>(&decode_hope)->default_value(false), "Decode dev input set according to hope objective")
		("decode-fear", po::value<bool>(&decode_fear)->default_value(false), "Decode dev input set according to fear objective")
		("decode-model", po::value<bool>(&decode_model)->default_value(false), "Decode dev input set according to normal objective")
		("decode-filename", po::value<string>(&decode_filename), "Filename for Bleu objective translations")
		("decoder-settings", po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
		("decr-learning-rate", po::value<float>(&decrease_learning_rate)->default_value(0),"Decrease learning rate by the given value after every epoch")
		("delay-updates", po::value<bool>(&delayUpdates)->default_value(false), "Delay all updates until the end of an epoch")
		("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(true), "Use n-best list with distinct translations in inference step")
		("epochs,e", po::value<size_t>(&epochs)->default_value(10), "Number of epochs")
		("fear-n", po::value<int>(&fear_n)->default_value(-1), "Number of fear translations used")
		("help", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
		("history-of-1best", po::value<bool>(&historyOf1best)->default_value(false), "Use 1best translations to update the history")
		("history-of-oracles", po::value<bool>(&historyOfOracles)->default_value(false), "Use oracle translations to update the history")
		("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.7), "Adjust the factor for history smoothing")
		("hope-fear", po::value<bool>(&hope_fear)->default_value(true), "Use only hope and fear translations for optimisation (not model)")
		("hope-fear-rank", po::value<bool>(&hope_fear_rank)->default_value(false), "Use hope and fear translations for optimisation, use model for ranking")
		("hope-model", po::value<bool>(&hope_model)->default_value(false), "Use only hope and model translations for optimisation (use --fear-n to set number of model translations)")
		("hope-n", po::value<int>(&hope_n)->default_value(-1), "Number of hope translations used")
		("input-file,i", po::value<string>(&inputFile), "Input file containing tokenised source")
		("learner,l", po::value<string>(&learner)->default_value("mira"), "Learning algorithm")
		("log-feature-values", po::value<bool>(&logFeatureValues)->default_value(false), "Take log of feature values according to the given base.")
		("margin-incr", po::value<float>(&margin_slack_incr)->default_value(0), "Increment margin slack after every epoch by this amount")
		("margin-slack", po::value<float>(&margin_slack)->default_value(0), "Slack when comparing left and right hand side of constraints")
		("max-length-dev-all", po::value<float>(&max_length_dev_all)->default_value(-1), "Make use of all 3 following options")
		("max-length-dev-hypos", po::value<float>(&max_length_dev_hypos)->default_value(-1), "Number between 0 and 1 specifying the percentage of admissible length deviation between hope and fear translations")
		("max-length-dev-hope-ref", po::value<float>(&max_length_dev_hope_ref)->default_value(-1), "Number between 0 and 1 specifying the percentage of admissible length deviation between hope and reference translations")
		("max-length-dev-fear-ref", po::value<float>(&max_length_dev_fear_ref)->default_value(-1), "Number between 0 and 1 specifying the percentage of admissible length deviation between fear and reference translations")
		("min-bleu-ratio", po::value<float>(&minBleuRatio)->default_value(-1), "Set a minimum BLEU ratio between hope and fear")
		("max-bleu-ratio", po::value<float>(&maxBleuRatio)->default_value(-1), "Set a maximum BLEU ratio between hope and fear")
		("min-learning-rate", po::value<float>(&min_learning_rate)->default_value(0), "Set a minimum learning rate")
		("min-oracle-bleu", po::value<float>(&min_oracle_bleu)->default_value(0), "Set a minimum oracle BLEU score")
		("min-weight-change", po::value<float>(&min_weight_change)->default_value(0.01), "Set minimum weight change for stopping criterion")
		("mira-learning-rate", po::value<float>(&mira_learning_rate)->default_value(1), "Learning rate for MIRA (fixed or flexible)")
		("mixing-frequency", po::value<size_t>(&mixingFrequency)->default_value(5), "How often per epoch to mix weights, when using mpi")
		("model-hope-fear", po::value<bool>(&model_hope_fear)->default_value(false), "Use model, hope and fear translations for optimisation")
		("nbest,n", po::value<size_t>(&n)->default_value(1), "Number of translations in n-best list")
		("normalise", po::value<bool>(&normaliseWeights)->default_value(false), "Whether to normalise the updated weights before passing them to the decoder")
		("normalise-margin", po::value<bool>(&normaliseMargin)->default_value(false), "Normalise the margin: squash between 0 and 1")
		("only-violated-constraints", po::value<bool>(&onlyViolatedConstraints)->default_value(false), "Add only violated constraints to the optimisation problem")
		("perceptron-learning-rate", po::value<float>(&perceptron_learning_rate)->default_value(0.01), "Perceptron learning rate")
		("print-feature-values", po::value<bool>(&print_feature_values)->default_value(false), "Print out feature values")
		("rank-n", po::value<int>(&rank_n)->default_value(-1), "Number of translations used for ranking")
		("rank-only", po::value<bool>(&rank_only)->default_value(false), "Use only model translations for optimisation")
		("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training")
		("relax-BP", po::value<float>(&relax_BP)->default_value(1), "Relax the BP by setting this value between 0 and 1")
		("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale the BLEU score by (a history of) the input length")
		("scale-by-reference-length", po::value<bool>(&scaleByReferenceLength)->default_value(false), "Scale BLEU by (a history of) the reference length")
		("scale-by-target-length-linear", po::value<bool>(&scaleByTargetLengthLinear)->default_value(false), "Scale BLEU by (a history of) the target length (linear future estimate)")
		("scale-by-target-length-trend", po::value<bool>(&scaleByTargetLengthTrend)->default_value(false), "Scale BLEU by (a history of) the target length (trend-based future estimate)")
		("scale-by-avg-length", po::value<bool>(&scaleByAvgLength)->default_value(false), "Scale BLEU by (a history of) the average of input and reference length")
		("scale-by-x", po::value<float>(&scaleByX)->default_value(1), "Scale the BLEU score by value x")
		("scale-margin", po::value<size_t>(&scale_margin)->default_value(0), "Scale the margin by the Bleu score of the oracle translation")
		("scale-update", po::value<size_t>(&scale_update)->default_value(0), "Scale the update by the Bleu score of the oracle translation")       
		("sentence-level-bleu", po::value<bool>(&sentenceLevelBleu)->default_value(true), "Use a sentences level Bleu scoring function")
		("shuffle", po::value<bool>(&shuffle)->default_value(false), "Shuffle input sentences before processing")
		("slack-min", po::value<float>(&slack_min)->default_value(0.01), "Minimum slack used")
		("slack-step", po::value<float>(&slack_step)->default_value(0), "Increase slack from epoch to epoch by the value provided")
		("stop-weights", po::value<bool>(&weightConvergence)->default_value(true), "Stop when weights converge")
		("threads", po::value<int>(&threadcount)->default_value(1), "Number of threads used")
		("update-scheme", po::value<size_t>(&update_scheme)->default_value(1), "Update scheme, default: 1")
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
/*#ifdef WITH_THREADS
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
#endif*/

  if (dummy != -1)
    slack = dummy;

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

	if (scaleByReferenceLength || scaleByTargetLengthLinear || scaleByTargetLengthTrend || scaleByAvgLength)
		scaleByInputLength = false;

	// initialise Moses
	// add initial Bleu weight and references to initialize Bleu feature
	decoder_settings += " -weight-bl 1 -references";
	for (size_t i=0; i < referenceFiles.size(); ++i) {
		decoder_settings += " ";
		decoder_settings += referenceFiles[i];
	}
	vector<string> decoder_params;
	boost::split(decoder_params, decoder_settings, boost::is_any_of("\t "));
	MosesDecoder* decoder = new MosesDecoder(mosesConfigFile, verbosity, decoder_params.size(), decoder_params);
	decoder->setBleuParameters(scaleByInputLength, scaleByReferenceLength, scaleByAvgLength,
			scaleByTargetLengthLinear, scaleByTargetLengthTrend,
			scaleByX, historySmoothing, bleu_smoothing_scheme, relax_BP);
	if (normaliseWeights) {
		ScoreComponentCollection startWeights = decoder->getWeights();
		startWeights.L1Normalise();
		decoder->setWeights(startWeights);
	}

	if (decode_hope || decode_fear || decode_model) {
		decodeHopeOrFear(decode_hope, decode_fear, decode_model, decode_filename, inputSentences, decoder, n);
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
			cerr << "slack: " << slack << ", learning rate: " << mira_learning_rate << endl;
		}
		optimiser = new MiraOptimiser(onlyViolatedConstraints, slack, scale_margin, scale_update,
				margin_slack, boost, update_scheme, normaliseMargin);
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
		rank_only = false; // mira only
		hope_fear = false; // mira only
		hope_fear_rank = false; // mira only
		hope_model = false; // mira only
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

	if (hope_n == -1)
		hope_n = n;
	if (fear_n == -1)
		fear_n = n;
	if (rank_n == -1)
		rank_n = n;

	if (model_hope_fear && hope_fear)
		hope_fear = false; // is true by default
	if (rank_only && hope_fear)
		hope_fear = false; // is true by default
	if (hope_fear_rank && hope_fear)
		hope_fear = false; // is true by default
	if (hope_model && hope_fear)
		hope_fear = false; // is true by default
	if (learner == "mira" && !(hope_fear || hope_model || model_hope_fear || rank_only || hope_fear_rank)) {
		cerr << "Error: Need to select an one of parameters --hope-fear/--model-hope-fear for mira update." << endl;
		return 1;
	}
	if (historyOf1best || historyOfOracles)
		sentenceLevelBleu = false;
	if (!sentenceLevelBleu) {
		if (!historyOf1best && !historyOfOracles) {
			historyOf1best = true;
		}
	}
	if (bleuScoreWeight_hope == -1) {
		bleuScoreWeight_hope = bleuScoreWeight;
	}
	if (bleuScoreWeight_fear == -1) {
		bleuScoreWeight_fear = bleuScoreWeight;
	}

	if (max_length_dev_all != -1) {
		max_length_dev_hypos = max_length_dev_all;
		max_length_dev_hope_ref = max_length_dev_all;
		max_length_dev_fear_ref = max_length_dev_all;
	}

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

	// get reference to feature functions
	const vector<const ScoreProducer*> featureFunctions =
		    StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT).GetFeatureFunctions();

	// read core weight file
	ProducerWeightMap coreWeightMap;
	if (!coreWeightFile.empty()) {
		if (!loadCoreWeights(coreWeightFile, coreWeightMap, featureFunctions)) {
				cerr << "Error: Failed to load core weights from " << coreWeightFile << endl;
				return 1;
		}
		else
			cerr << "Loaded core weights from " << coreWeightFile << "." << endl;
	}

	// set core weights
	ScoreComponentCollection initialWeights = decoder->getWeights();
	if (coreWeightMap.size() > 0) {
		ProducerWeightMap::iterator p;
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

	// for accumulating delayed updates
	ScoreComponentCollection delayedWeightUpdates;

	bool stop = false;
//	int sumStillViolatedConstraints;
	float *sendbuf, *recvbuf;
	sendbuf = (float *) malloc(sizeof(float));
	recvbuf = (float *) malloc(sizeof(float));
	for (size_t epoch = 0; epoch < epochs && !stop; ++epoch) {
		// sum of violated constraints in an epoch
//		sumStillViolatedConstraints = 0;

		numberOfUpdatesThisEpoch = 0;
		// Sum up weights over one epoch, final average uses weights from last epoch
		if (!accumulateWeights)
			cumulativeWeights.ZeroAll();

		delayedWeightUpdates.ZeroAll();

		// number of weight dumps this epoch
		size_t weightEpochDump = 0;

		size_t shardPosition = 0;
		vector<size_t>::const_iterator sid = shard.begin();
		while (sid != shard.end()) {
			// feature values for hypotheses i,j (matrix: batchSize x 3*n x featureValues)
			vector<vector<ScoreComponentCollection> > featureValues;
			vector<vector<float> > bleuScores;
			vector<vector<float> > modelScores;

			// variables for hope-fear/perceptron setting
			vector<vector<ScoreComponentCollection> > featureValuesHope;
			vector<vector<ScoreComponentCollection> > featureValuesFear;
			vector<vector<float> > bleuScoresHope;
			vector<vector<float> > bleuScoresFear;
			vector<vector<float> > modelScoresHope;
			vector<vector<float> > modelScoresFear;
			vector<vector<ScoreComponentCollection> > dummyFeatureValues;
			vector<vector<float> > dummyBleuScores;
			vector<vector<float> > dummyModelScores;

			// get moses weights
			ScoreComponentCollection mosesWeights = decoder->getWeights();
			VERBOSE(1, "\nRank " << rank << ", epoch " << epoch << ", weights: " << mosesWeights << endl);

			// BATCHING: produce nbest lists for all input sentences in batch
			vector<float> oracleBleuScores;
			vector<float> oracleModelScores;
			vector<vector<const Word*> > oracles;
			vector<vector<const Word*> > oneBests;
			vector<ScoreComponentCollection> oracleFeatureValues;
			vector<size_t> inputLengths;
			vector<size_t> ref_ids;
			size_t actualBatchSize = 0;

			vector<size_t>::const_iterator current_sid_start = sid;
			size_t examples_in_batch = 0;
			for (size_t batchPosition = 0; batchPosition < batchSize && sid
			    != shard.end(); ++batchPosition) {
				string& input = inputSentences[*sid];
//				const vector<string>& refs = referenceSentences[*sid];
				cerr << "\nRank " << rank << ", epoch " << epoch << ", input sentence " << *sid << ": \"" << input << "\"" << " (batch pos " << batchPosition << ")" << endl;

				vector<ScoreComponentCollection> newFeatureValues;
				vector<float> newScores;
				if (model_hope_fear || rank_only || hope_fear_rank) {
					featureValues.push_back(newFeatureValues);
					bleuScores.push_back(newScores);
					modelScores.push_back(newScores);
				}
				if (hope_fear || hope_model || hope_fear_rank || perceptron_update) {
					featureValuesHope.push_back(newFeatureValues);
					featureValuesFear.push_back(newFeatureValues);
					bleuScoresHope.push_back(newScores);
					bleuScoresFear.push_back(newScores);
					modelScoresHope.push_back(newScores);
					modelScoresFear.push_back(newScores);
					if (historyOf1best) {
						dummyFeatureValues.push_back(newFeatureValues);
						dummyBleuScores.push_back(newScores);
						dummyModelScores.push_back(newScores);
					}
				}

				size_t ref_length;
				float avg_ref_length;
				if (hope_fear || hope_fear_rank || perceptron_update) {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << hope_n << "best hope translations" << endl;
					vector< vector<const Word*> > outputHope = decoder->getNBest(input, *sid, hope_n, 1.0, bleuScoreWeight_hope,
							featureValuesHope[batchPosition], bleuScoresHope[batchPosition], modelScoresHope[batchPosition],
							1, distinctNbest, rank, epoch);
					vector<const Word*> oracle = outputHope[0];
					size_t current_input_length = decoder->getCurrentInputLength();
					decoder->cleanup();
					ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
					avg_ref_length = ref_length;
					float hope_length_ratio = (float)oracle.size()/ref_length;
					int oracleSize = (int)oracle.size();
					cerr << ", l-ratio hope: " << hope_length_ratio << endl;
					cerr << "Rank " << rank << ", epoch " << epoch << ", current input length: " << current_input_length << endl;

					bool skip = false;

					// Length-related example selection
					float length_diff_hope = abs(1 - hope_length_ratio);
					if (max_length_dev_hope_ref != -1 && length_diff_hope > max_length_dev_hope_ref)
					  skip = true;

					vector<const Word*> bestModel;
					if (historyOf1best && !skip) {
						// MODEL (for updating the history only, using dummy vectors)
						cerr << "Rank " << rank << ", epoch " << epoch << ", 1best wrt model score (for history or length stabilisation)" << endl;
						vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, 1, 0.0, bleuScoreWeight,
								dummyFeatureValues[batchPosition], dummyBleuScores[batchPosition], dummyModelScores[batchPosition],
								1, distinctNbest, rank, epoch);
						bestModel = outputModel[0];
						decoder->cleanup();
						cerr << endl;
						ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					}

					// FEAR
					float fear_length_ratio = 0;
					float bleuRatioHopeFear = 0;
					int fearSize = 0;
					if (!skip) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", " << fear_n << "best fear translations" << endl;
						vector< vector<const Word*> > outputFear = decoder->getNBest(input, *sid, fear_n, -1.0, bleuScoreWeight_fear,
								featureValuesFear[batchPosition], bleuScoresFear[batchPosition], modelScoresFear[batchPosition],
								1,	distinctNbest, rank, epoch);
						vector<const Word*> fear = outputFear[0];
						decoder->cleanup();
						ref_length = decoder->getClosestReferenceLength(*sid, fear.size());
						avg_ref_length += ref_length;
						avg_ref_length /= 2;
						fear_length_ratio = (float)fear.size()/ref_length;
						fearSize = (int)fear.size();
						cerr << ", l-ratio fear: " << fear_length_ratio << endl;
						for (size_t i = 0; i < fear.size(); ++i)
							delete fear[i];

						// Bleu-related example selection
						bleuRatioHopeFear = bleuScoresHope[batchPosition][0] / bleuScoresFear[batchPosition][0];
						if (minBleuRatio != -1 && bleuRatioHopeFear < minBleuRatio)
							skip = true;
						if(maxBleuRatio != -1 && bleuRatioHopeFear > maxBleuRatio)
							skip = true;

						// Length-related example selection
						float length_diff_fear = abs(1 - fear_length_ratio);
						size_t length_diff_hope_fear = abs(oracleSize - fearSize);
						cerr << "Rank " << rank << ", epoch " << epoch << ", abs-length hope-fear: " << length_diff_hope_fear << ", BLEU hope-fear: " << bleuScoresHope[batchPosition][0] - bleuScoresFear[batchPosition][0] << endl;
						if (max_length_dev_hypos != -1 && (length_diff_hope_fear > avg_ref_length * max_length_dev_hypos))
							skip = true;

						if (max_length_dev_fear_ref != -1 && length_diff_fear > max_length_dev_fear_ref)
							skip = true;
					}
					
					if (skip) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", skip example (" << hope_length_ratio << ", " << bleuRatioHopeFear << ").. " << endl;
						featureValuesHope[batchPosition].clear();
						featureValuesFear[batchPosition].clear();
						bleuScoresHope[batchPosition].clear();
						bleuScoresFear[batchPosition].clear();
						if (historyOf1best) {
							dummyFeatureValues[batchPosition].clear();
							dummyBleuScores[batchPosition].clear();
						}
					}
					else {
						// needed for history
						inputLengths.push_back(current_input_length);
						ref_ids.push_back(*sid);

						if (!sentenceLevelBleu) {
							oracles.push_back(oracle);
							oneBests.push_back(bestModel);
						}

						examples_in_batch++;
					}
				}
				if (hope_model) {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << hope_n << "best hope translations" << endl;
					vector< vector<const Word*> > outputHope = decoder->getNBest(input, *sid, hope_n, 1.0, bleuScoreWeight_hope,
							featureValuesHope[batchPosition], bleuScoresHope[batchPosition], modelScoresHope[batchPosition],
							1, distinctNbest, rank, epoch);
					vector<const Word*> oracle = outputHope[0];
					size_t current_input_length = decoder->getCurrentInputLength();
					decoder->cleanup();
					cerr << endl;

					vector<const Word*> bestModel;
					// MODEL (for updating the history only, using dummy vectors)
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << fear_n << "best wrt model score" << endl;
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, fear_n, 0.0, bleuScoreWeight_fear,
							featureValuesFear[batchPosition], bleuScoresFear[batchPosition], modelScoresFear[batchPosition],
							1, distinctNbest, rank, epoch);
					bestModel = outputModel[0];
					decoder->cleanup();
					cerr << endl;

					// needed for history
					inputLengths.push_back(current_input_length);
					ref_ids.push_back(*sid);

					if (!sentenceLevelBleu) {
						oracles.push_back(oracle);
						oneBests.push_back(bestModel);
					}

					examples_in_batch++;
				}
				if (rank_only || hope_fear_rank) {
					// MODEL
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << rank_n << "best wrt model score" << endl;
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, rank_n, 0.0, bleuScoreWeight,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, rank, epoch);
					vector<const Word*> bestModel = outputModel[0];
					decoder->cleanup();
					oneBests.push_back(bestModel);
					ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					float model_length_ratio = (float)bestModel.size()/ref_length;
					cerr << ", l-ratio model: " << model_length_ratio << endl;

					examples_in_batch++;
				}
				if (model_hope_fear) {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best hope translations" << endl;
					size_t oraclePos = featureValues[batchPosition].size();
					vector <vector<const Word*> > outputHope = decoder->getNBest(input, *sid, n, 1.0, bleuScoreWeight_hope,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, rank, epoch);
					vector<const Word*> oracle = outputHope[0];
					// needed for history
					inputLengths.push_back(decoder->getCurrentInputLength());
					ref_ids.push_back(*sid);
					decoder->cleanup();
					oracles.push_back(oracle);
					ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
					float hope_length_ratio = (float)oracle.size()/ref_length;
					cerr << ", l-ratio hope: " << hope_length_ratio << endl;

					oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
					oracleBleuScores.push_back(bleuScores[batchPosition][oraclePos]);
					oracleModelScores.push_back(modelScores[batchPosition][oraclePos]);

					// MODEL
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best wrt model score" << endl;
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, n, 0.0, bleuScoreWeight,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, rank, epoch);
					vector<const Word*> bestModel = outputModel[0];
					decoder->cleanup();
					oneBests.push_back(bestModel);
					ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					float model_length_ratio = (float)bestModel.size()/ref_length;
					cerr << ", l-ratio model: " << model_length_ratio << endl;

					// FEAR
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best fear translations" << endl;
					size_t fearPos = featureValues[batchPosition].size();
					vector< vector<const Word*> > outputFear = decoder->getNBest(input, *sid, n, -1.0, bleuScoreWeight_fear,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1, distinctNbest, rank, epoch);
					vector<const Word*> fear = outputFear[0];
					decoder->cleanup();
					ref_length = decoder->getClosestReferenceLength(*sid, fear.size());
					float fear_length_ratio = (float)fear.size()/ref_length;
					cerr << ", l-ratio fear: " << fear_length_ratio << endl;
					for (size_t i = 0; i < fear.size(); ++i) {
						delete fear[i];
					}

					examples_in_batch++;
				}

				// next input sentence
				++sid;
				++actualBatchSize;
				++shardPosition;
			} // end of batch loop


			if (examples_in_batch == 0) {
				cerr << "Rank " << rank << ", epoch " << epoch << ", batch is empty." << endl;
			}
			else {
				vector<vector<float> > losses(actualBatchSize);
				if (model_hope_fear) {
					// Set loss for each sentence as BLEU(oracle) - BLEU(hypothesis)
					for (size_t batchPosition = 0; batchPosition < actualBatchSize; ++batchPosition) {
						for (size_t j = 0; j < bleuScores[batchPosition].size(); ++j) {
							losses[batchPosition].push_back(oracleBleuScores[batchPosition] - bleuScores[batchPosition][j]);
						}
					}
				}

				// set weight for bleu feature to 0 before optimizing
				vector<const ScoreProducer*>::const_iterator iter = featureFunctions.begin();
				for (; iter != featureFunctions.end(); ++iter)
				  if ((*iter)->GetScoreProducerWeightShortName() == "bl") {
				    mosesWeights.Assign(*iter, 0);
				    break;
				  }

				// take logs of feature values
				if (logFeatureValues) {
					takeLogs(featureValuesHope, baseOfLog);
					takeLogs(featureValuesFear, baseOfLog);
					takeLogs(featureValues, baseOfLog);
					for (size_t i = 0; i < oracleFeatureValues.size(); ++i) {
						oracleFeatureValues[i].LogCoreFeatures(baseOfLog);
					}
				}

				// print out the feature values
				if (print_feature_values) {
					cerr << "\nRank " << rank << ", epoch " << epoch << ", feature values: " << endl;
					if (model_hope_fear || rank_only) printFeatureValues(featureValues);
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
				ScoreComponentCollection weightUpdate;
				if (perceptron_update) {
					vector<vector<float> > dummy1;
					update_status = optimiser->updateWeightsHopeFear(mosesWeights, weightUpdate,
							featureValuesHope, featureValuesFear, dummy1, dummy1, dummy1, dummy1, learning_rate, rank, epoch);
				}
				else if (hope_fear || hope_model) {
					if (bleuScoresHope[0][0] >= min_oracle_bleu)
						if (hope_n == 1 && fear_n ==1)
							update_status = ((MiraOptimiser*) optimiser)->updateWeightsAnalytically(mosesWeights, weightUpdate,
									featureValuesHope[0][0], featureValuesFear[0][0], bleuScoresHope[0][0], bleuScoresFear[0][0],
									modelScoresHope[0][0], modelScoresFear[0][0], learning_rate, rank, epoch);
						else
							update_status = optimiser->updateWeightsHopeFear(mosesWeights, weightUpdate,
									featureValuesHope, featureValuesFear, bleuScoresHope, bleuScoresFear,
									modelScoresHope, modelScoresFear, learning_rate, rank, epoch);
				  else
				    update_status = -1;										     
				}
				else if (rank_only) {
					// learning ranking of model translations
					update_status = ((MiraOptimiser*) optimiser)->updateWeightsRankModel(mosesWeights, weightUpdate,
							featureValues, bleuScores, modelScores, learning_rate, rank, epoch);
				}
				else if (hope_fear_rank) {
					// hope-fear + learning ranking of model translations
					update_status = ((MiraOptimiser*) optimiser)->updateWeightsHopeFearAndRankModel(mosesWeights, weightUpdate,
							featureValuesHope, featureValuesFear, featureValues, bleuScoresHope, bleuScoresFear, bleuScores,
							modelScoresHope, modelScoresFear, modelScores, learning_rate, rank, epoch);
				}
				else {
					// model_hope_fear
					update_status = ((MiraOptimiser*) optimiser)->updateWeights(mosesWeights, weightUpdate,
							featureValues, losses, bleuScores, modelScores, oracleFeatureValues, oracleBleuScores, oracleModelScores, learning_rate, rank, epoch);
				}

//			sumStillViolatedConstraints += update_status;

				if (update_status == 0) {	 // if weights were updated
					// apply weight update
					if (delayUpdates) {
						delayedWeightUpdates.PlusEquals(weightUpdate);
						//						cerr << "\nRank " << rank << ", epoch " << epoch << ", keeping update: " << weightUpdate << endl;
						++numberOfUpdatesThisEpoch;
					}
					else {
					  cerr << "Rank " << rank << ", epoch " << epoch << ", applying update.." << endl;
						mosesWeights.PlusEquals(weightUpdate);
						if (normaliseWeights)
							mosesWeights.L1Normalise();

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

						if (!delayUpdates)
							// set new Moses weights
							decoder->setWeights(mosesWeights);
					}
				}

				// update history (for approximate document Bleu)
				if (historyOf1best) {
					for (size_t i = 0; i < oneBests.size(); ++i) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", update history with 1best length: " << oneBests[i].size() << " ";
						}
					decoder->updateHistory(oneBests, inputLengths, ref_ids, rank, epoch);
				}
				else if (historyOfOracles) {
					for (size_t i = 0; i < oracles.size(); ++i) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", update history with oracle length: " << oracles[i].size() << " ";
					}
					decoder->updateHistory(oracles, inputLengths, ref_ids, rank, epoch);
				}
				deleteTranslations(oracles);
				deleteTranslations(oneBests);
			} // END TRANSLATE AND UPDATE OF BATCH

			size_t mixing_base = mixingFrequency == 0 ? 0 : shard.size() / mixingFrequency;
			size_t dumping_base = weightDumpFrequency ==0 ? 0 : shard.size() / weightDumpFrequency;
			// mix weights?
			if (evaluateModulo(shardPosition, mixing_base, actualBatchSize)) {
#ifdef MPI_ENABLE
				ScoreComponentCollection mixedWeights;
				//				cerr << "\nRank " << rank << ", before mixing: " << mosesWeights << endl;

				// collect all weights in mixedWeights and divide by number of processes
				mpi::reduce(world, mosesWeights, mixedWeights, SCCPlus(), 0);
				if (rank == 0) {
					// divide by number of processes
					mixedWeights.DivideEquals(size);

					// normalise weights after averaging
					if (normaliseWeights) {
						mixedWeights.L1Normalise();
						//						cerr << "Mixed weights (normalised): " << mixedWeights << endl;
					}
					else {
					  //						cerr << "Mixed weights: " << mixedWeights << endl;
					}
				}

				// broadcast average weights from process 0
				mpi::broadcast(world, mixedWeights, 0);
				decoder->setWeights(mixedWeights);
				mosesWeights = mixedWeights;
#endif
#ifndef MPI_ENABLE
				//				cerr << "\nRank " << rank << ", no mixing, weights: " << mosesWeights << endl;
#endif
			} // end mixing

			// Dump weights?
			if (!delayUpdates && evaluateModulo(shardPosition, dumping_base, actualBatchSize)) {
			  ScoreComponentCollection tmpAverageWeights(cumulativeWeights);
			  bool proceed = false;
			  if (accumulateWeights) {
			    if (numberOfUpdates > 0) {
			      tmpAverageWeights.DivideEquals(numberOfUpdates);
			      proceed = true;
			    }
			  } else {
			    if (numberOfUpdatesThisEpoch > 0) {
			      tmpAverageWeights.DivideEquals(numberOfUpdatesThisEpoch);
			      proceed = true;
			    }
			  }
			  
			  if (proceed) {
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
				//			      	cerr << "\nMixed average weights (cumulative) during epoch "	<< epoch << ": " << mixedAverageWeights << endl;
			      } else {
				//			      	cerr << "\nMixed average weights during epoch " << epoch << ": " << mixedAverageWeights << endl;
			      }
			      
			      cerr << "Dumping mixed average weights during epoch " << epoch << " to " << filename.str() << endl << endl;
			      mixedAverageWeights.Save(filename.str());
			      ++weightEpochDump;
			    }
			  }
			}// end dumping

		} // end of shard loop, end of this epoch

		if (delayUpdates) {
			// apply all updates from this epoch to the weight vector
			ScoreComponentCollection mosesWeights = decoder->getWeights();
			cerr << "Rank " << rank << ", epoch " << epoch << ", delayed update, old moses weights: " << mosesWeights << endl;
			mosesWeights.PlusEquals(delayedWeightUpdates);
			cumulativeWeights.PlusEquals(mosesWeights);
			decoder->setWeights(mosesWeights);
			cerr << "Rank " << rank << ", epoch " << epoch << ", delayed update, new moses weights: " << mosesWeights << endl;

		  ScoreComponentCollection tmpAverageWeights(cumulativeWeights);
		  bool proceed = false;
		  if (accumulateWeights) {
		    if (numberOfUpdatesThisEpoch > 0) {
		      tmpAverageWeights.DivideEquals(epoch+1);
		      proceed = true;
		    }
		  }
		  else {
		    if (numberOfUpdatesThisEpoch > 0)
		      proceed = true;
		  }

		  if (proceed) {
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
		  }
		}

		if (verbosity > 0) {
			cerr << "Bleu feature history after epoch " <<  epoch << endl;
			decoder->printBleuFeatureHistory(cerr);
		}
//		cerr << "Rank " << rank << ", epoch " << epoch << ", sum of violated constraints: " << sumStillViolatedConstraints << endl;

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
					VERBOSE(1, "Average weight changes since previous epoch: " << firstDiff << 
						" (max: " << firstDiff.GetLInfNorm() << ")" << endl);
					ScoreComponentCollection secondDiff(mixedAverageWeights);
					secondDiff.MinusEquals(mixedAverageWeightsBeforePrevious);
					VERBOSE(1, "Average weight changes since before previous epoch: " << secondDiff << 
						" (max: " << secondDiff.GetLInfNorm() << ")" << endl << endl);

					// check whether stopping criterion has been reached
					// (both difference vectors must have all weight changes smaller than min_weight_change)
					if (firstDiff.GetLInfNorm() >= min_weight_change)
					  reached = false;
					if (secondDiff.GetLInfNorm() >= min_weight_change)
					  reached = false;
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

bool loadCoreWeights(const string& filename, ProducerWeightMap& coreWeightMap, const vector<const ScoreProducer*> &featureFunctions) {
	ifstream in(filename.c_str());
	if (!in)
		return false;
	string line;
	vector< float > store_weights;
	cerr << "Loading core weights:" << endl;
	while (getline(in, line)) {
		// split weight name from value
		vector<string> split_line;
		boost::split(split_line, line, boost::is_any_of(" "));
		float weight;
		if(!from_string<float>(weight, split_line[1], std::dec))
		{
			cerr << "reading in float failed.." << endl;
			return false;
		}

		// find producer for this score
		string name = split_line[0];
		for (size_t i=0; i < featureFunctions.size(); ++i) {
			std::string prefix = featureFunctions[i]->GetScoreProducerDescription();
			if (name.substr( 0, prefix.length() ).compare( prefix ) == 0) {
				if (featureFunctions[i]->GetNumScoreComponents() == 1) {
					vector< float > weights;
					weights.push_back(weight);
					coreWeightMap.insert(ProducerWeightPair(featureFunctions[i], weights));
					cerr << "insert 1 weight for " << featureFunctions[i]->GetScoreProducerDescription();
					cerr << " (" << weight << ")" << endl;
				}
				else {
					store_weights.push_back(weight);
					if (store_weights.size() == featureFunctions[i]->GetNumScoreComponents()) {
						coreWeightMap.insert(ProducerWeightPair(featureFunctions[i], store_weights));
						cerr << "insert " << store_weights.size() << " weights for " << featureFunctions[i]->GetScoreProducerDescription() << " (";
						for (size_t j=0; j < store_weights.size(); ++j)
							cerr << store_weights[j] << " ";
						cerr << ")" << endl;
						store_weights.clear();
					}
				}
			}
		}
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

void ignoreCoreFeatures(vector<vector<ScoreComponentCollection> > &featureValues, ProducerWeightMap &coreWeightMap) {
	for (size_t i = 0; i < featureValues.size(); ++i)
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			// set all core features to 0
			ProducerWeightMap::iterator p;
			for(p = coreWeightMap.begin(); p!=coreWeightMap.end(); ++p) {
				if ((p->first)->GetNumScoreComponents() == 1)
					featureValues[i][j].Assign(p->first, 0);
				else {
					vector< float > weights;
					for (size_t k=0; k < (p->first)->GetNumScoreComponents(); ++k)
						weights.push_back(0);
					featureValues[i][j].Assign(p->first, weights);
				}
			}
		}
}

void takeLogs(vector<vector<ScoreComponentCollection> > &featureValues, size_t base) {
	for (size_t i = 0; i < featureValues.size(); ++i) {
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			featureValues[i][j].LogCoreFeatures(base);
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

void decodeHopeOrFear(bool decode_hope, bool decode_fear, bool decode_model, string filename, vector<string> &inputSentences, MosesDecoder* decoder, size_t n) {
	if (decode_hope)
		cerr << "Decoding dev input set according to hope objective.. " << endl;
	else if (decode_fear)
		cerr << "Decoding dev input set according to fear objective.. " << endl;
	else
		cerr << "Decoding dev input set according to normal objective.. " << endl;

	// open files for writing
	ostringstream filename_nbest;
	filename_nbest << filename << "." << n << "best";
	ofstream out(filename.c_str());
  ofstream nbest_out((filename_nbest.str()).c_str());
  if (!out) {
    ostringstream msg;
    msg << "Unable to open " << filename;
    throw runtime_error(msg.str());
  }
  if (!nbest_out) {
    ostringstream msg;
    msg << "Unable to open " << filename_nbest;
    throw runtime_error(msg.str());
  }

  for (size_t sid = 0; sid < inputSentences.size(); ++sid) {
		string& input = inputSentences[sid];

		vector<vector<ScoreComponentCollection> > dummyFeatureValues;
		vector<vector<float> > dummyBleuScores;
		vector<vector<float> > dummyModelScores;

		vector<ScoreComponentCollection> newFeatureValues;
		vector<float> newScores;
		dummyFeatureValues.push_back(newFeatureValues);
		dummyBleuScores.push_back(newScores);
		dummyModelScores.push_back(newScores);

		float factor = 0.0;
		if (decode_hope) factor = 1.0;
		if (decode_fear) factor = -1.0;
		vector< vector<const Word*> > nbestOutput = decoder->getNBest(input, sid, n, factor, 1, dummyFeatureValues[0],
				dummyBleuScores[0], dummyModelScores[0], n, true, 0, 0);
		cerr << endl;
		decoder->cleanup();

		for (size_t i = 0; i < nbestOutput.size(); ++i) {
			vector<const Word*> output = nbestOutput[i];
			stringstream translation;
			for (size_t k = 0; k < output.size(); ++k) {
				Word* w = const_cast<Word*>(output[k]);
				translation << w->GetString(0);
				translation << " ";
			}

			if (i == 0)
				out << translation.str() << endl;
			nbest_out << sid << " ||| " << translation.str() << " ||| " << dummyFeatureValues[0][i] <<
					" ||| " << dummyModelScores[0][i] << " ||| sBleu=" << dummyBleuScores[0][i] << endl;
		}
	}

  out.close();
  nbest_out.close();
  cerr << "Closing files " << filename << " and " << filename_nbest.str() << endl;

	exit(0);
}
