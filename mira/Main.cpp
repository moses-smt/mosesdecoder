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

void OutputNBestList(const MosesChart::TrellisPathList &nBestList,
    const TranslationSystem* system, long translationId);

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

bool evaluateModulo(size_t shard_position, size_t mix_or_dump_base, size_t actual_batch_size) {
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

struct RandomIndex {
	ptrdiff_t operator()(ptrdiff_t max) {
		return static_cast<ptrdiff_t> (rand() % max);
	}
};

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
	float min_learning_rate;
	float min_sentence_update;
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
	bool adapt_BPfactor;
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
	int updates_per_epoch;
	bool averageWeights;
	bool stop_optimal;
	po::options_description desc("Allowed options");
	desc.add_options()("accumulate-most-violated-constraints", po::value<bool>(&accumulateMostViolatedConstraints)->default_value(false),"Accumulate most violated constraint per example")
			("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
			("adapt-BP-factor", po::value<bool>(&adapt_BPfactor)->default_value(0), "Set factor to 1 when optimal translation length in reached")
			("average-weights", po::value<bool>(&averageWeights)->default_value(false), "Set decoder weights to average weights after each update")
			("base-of-log", po::value<size_t>(&baseOfLog)->default_value(10), "Base for log-ing feature values")
			("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
			("BP-factor", po::value<float>(&BPfactor)->default_value(1.0), "Increase penalty for short translations")
			("config,f", po::value<string>(&mosesConfigFile), "Moses ini file")
			("control-updates", po::value<bool>(&controlUpdates)->default_value(true), "Ignore updates that increase number of violated constraints AND increase the error")
			("decoder-settings", po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
			("decr-learning-rate", po::value<float>(&decrease_learning_rate)->default_value(0),"Decrease learning rate by the given value after every epoch")
			("decr-sentence-update", po::value<float>(&decrease_sentence_update)->default_value(0), "Decrease maximum weight update by the given value after every epoch")
			("dev-bleu", po::value<bool>(&devBleu)->default_value(true), "Compute BLEU score of oracle translations of the whole tuning set")
			("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(true), "Use nbest list with distinct translations in inference step")
			("weight-dump-frequency", po::value<size_t>(&weightDumpFrequency)->default_value(1), "How often per epoch to dump weights, when using mpi")
			("epochs,e", po::value<size_t>(&epochs)->default_value(5), "Number of epochs")
			("help", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
			("hildreth", po::value<bool>(&hildreth)->default_value(true), "Use Hildreth's optimisation algorithm")
			("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.9), "Adjust the factor for history smoothing")
			("input-file,i", po::value<string>(&inputFile), "Input file containing tokenised source")
			("learner,l", po::value<string>(&learner)->default_value("mira"), "Learning algorithm")
			("learning-rate", po::value<float>(&learning_rate)->default_value(1), "Learning rate (fixed or flexible)")
			("log-feature-values", po::value<bool>(&logFeatureValues)->default_value(false), "Take log of feature values according to the given base.")
			("max-number-oracles", po::value<size_t>(&maxNumberOracles)->default_value(1), "Set a maximum number of oracles to use per example")
			("min-sentence-update", po::value<float>(&min_sentence_update)->default_value(0), "Set a minimum weight update per sentence")
			("min-learning-rate", po::value<float>(&min_learning_rate)->default_value(0), "Set a minimum learning rate")
			("max-sentence-update", po::value<float>(&max_sentence_update)->default_value(-1), "Set a maximum weight update per sentence")
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
	    ("stop-dev-bleu", po::value<bool>(&stop_dev_bleu)->default_value(false), "Stop when average Bleu (dev) decreases (or no more increases)")
	    ("stop-approx-dev-bleu", po::value<bool>(&stop_approx_dev_bleu)->default_value(false), "Stop when average approx. sentence Bleu (dev) decreases (or no more increases)")
	    ("stop-weights", po::value<bool>(&weightConvergence)->default_value(false), "Stop when weights converge")
	    ("stop-optimal", po::value<bool>(&stop_optimal)->default_value(true), "Stop when the results of optimization do not improve further")
	    ("updates-per-epoch", po::value<int>(&updates_per_epoch)->default_value(-1), "Accumulate updates and apply them to the weight vector the specified number of times per epoch")
	    ("use-scaled-reference", po::value<bool>(&useScaledReference)->default_value(true), "Use scaled reference length for comparing target and reference length of phrases")
	    ("verbosity,v", po::value<int>(&verbosity)->default_value(0), "Verbosity level")
	    ("weighted-loss-function", po::value<bool>(&weightedLossFunction)->default_value(false), "Weight the loss of a hypothesis by its Bleu score")
	    ("weight-dump-stem", po::value<string>(&weightDumpStem)->default_value("weights"), "Stem of filename to use for dumping weights");

	po::options_description cmdline_options;
	cmdline_options.add(desc);
	po::variables_map vm;
	po::store(
	    po::command_line_parser(argc, argv). options(cmdline_options).run(), vm);
	po::notify(vm);

	if (help) {
		std::cout << "Usage: " + string(argv[0])
		    + " -f mosesini-file -i input-file -r reference-file(s) [options]"
		    << std::endl;
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

	// initialise Moses
	vector<string> decoder_params;
	boost::split(decoder_params, decoder_settings, boost::is_any_of("\t "));
	initMoses(mosesConfigFile, verbosity, decoder_params.size(), decoder_params);
	MosesDecoder* decoder = new MosesDecoder(referenceSentences,
	    useScaledReference, scaleByInputLength, BPfactor, historySmoothing);
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
	float shardSize = (float) (order.size()) / size;
	VERBOSE(1, "Shard size: " << shardSize << endl);
	size_t shardStart = (size_t) (shardSize * rank);
	size_t shardEnd = (size_t) (shardSize * (rank + 1));
	if (rank == size - 1)
		shardEnd = order.size();
	VERBOSE(1, "Rank: " << rank << " Shard start: " << shardStart << " Shard end: " << shardEnd << endl);
	shard.resize(shardSize);
	copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());

	Optimiser* optimiser = NULL;
	cerr << "adapt-BP-factor: " << adapt_BPfactor << endl;
	cerr << "control-updates: " << controlUpdates << endl;
	cerr << "mix-frequency: " << mixingFrequency << endl;
	cerr << "weight-dump-frequency: " << weightDumpFrequency << endl;
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
	cerr << "accumulate-most-violated-constraints: "
	    << accumulateMostViolatedConstraints << endl;
	cerr << "past-and-current-constraints: " << pastAndCurrentConstraints << endl;
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
	cerr << "stop-optimal: " << stop_optimal << endl;
	cerr << "stop-weights: " << weightConvergence << endl;
	cerr << "updates-per-epoch: " << updates_per_epoch << endl;
	cerr << "use-total-weights-for-pruning: " << averageWeights << endl;

	if (learner == "mira") {
		cerr << "Optimising using Mira" << endl;
		optimiser = new MiraOptimiser(n, hildreth, marginScaleFactor,
		    onlyViolatedConstraints, slack, weightedLossFunction, maxNumberOracles,
		    accumulateMostViolatedConstraints, pastAndCurrentConstraints,
		    order.size());
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
	// print initial weights
	cerr << "Rank " << rank << ", initial weights: " << decoder->getWeights() << endl;
	ScoreComponentCollection cumulativeWeights; // collect weights per epoch to produce an average
	size_t numberOfUpdates = 0;
	size_t numberOfUpdatesThisEpoch = 0;

	time_t now = time(0); // get current time
	struct tm* tm = localtime(&now); // get struct filled out
	cerr << "Start date/time: " << tm->tm_mon + 1 << "/" << tm->tm_mday << "/"
	    << tm->tm_year + 1900 << ", " << tm->tm_hour << ":" << tm->tm_min << ":"
	    << tm->tm_sec << endl;

	ScoreComponentCollection mixedAverageWeights;
	ScoreComponentCollection mixedAverageWeightsPrevious;
	ScoreComponentCollection mixedAverageWeightsBeforePrevious;

	float averageRatio = 0;
	float averageBleu = 0;
	float prevAverageBleu = 0;
	float beforePrevAverageBleu = 0;
	float summedApproxBleu = 0;
	float averageApproxBleu = 0;
	float prevAverageApproxBleu = 0;
	float beforePrevAverageApproxBleu = 0;
	bool stop = false;
	size_t sumStillViolatedConstraints;
	size_t sumStillViolatedConstraints_lastEpoch = 0;
	size_t sumConstraintChangeAbs;
	size_t sumConstraintChangeAbs_lastEpoch = 0;
	float *sendbuf, *recvbuf;
	sendbuf = (float *) malloc(sizeof(float));
	recvbuf = (float *) malloc(sizeof(float));
	// Note: make sure that the variable mosesWeights always holds the current decoder weights
	for (size_t epoch = 0; epoch < epochs && !stop; ++epoch) {
		cerr << "\nRank " << rank << ", epoch " << epoch << endl;

		// sum of violated constraints
		sumStillViolatedConstraints = 0;
		sumConstraintChangeAbs = 0;

		// sum of approx. sentence bleu scores per epoch
		summedApproxBleu = 0;

		numberOfUpdatesThisEpoch = 0;
		// Sum up weights over one epoch, final average uses weights from last epoch
		if (!accumulateWeights) {
			cumulativeWeights.ZeroAll();
		}

		// number of weight dumps this epoch
		size_t weightEpochDump = 0;

		// collect best model score translations for computing bleu on dev set
		vector<vector<const Word*> > allBestModelScore;
		vector<size_t> all_ref_ids;

		size_t shardPosition = 0;
		vector<size_t>::const_iterator sid = shard.begin();
		while (sid != shard.end()) {
			// feature values for hypotheses i,j (matrix: batchSize x 3*n x featureValues)
			vector<vector<ScoreComponentCollection> > featureValues;
			vector<vector<float> > bleuScores;

			// get moses weights
			ScoreComponentCollection mosesWeights = decoder->getWeights();
			cerr << "\nRank " << rank << ", next batch" << endl;
			cerr << "Rank " << rank << ", weights: " << mosesWeights << endl;

			// BATCHING: produce nbest lists for all input sentences in batch
			vector<size_t> oraclePositions;
			vector<float> oracleBleuScores;
			vector<vector<const Word*> > oracles;
			vector<ScoreComponentCollection> oracleFeatureValues;
			vector<size_t> inputLengths;
			vector<size_t> ref_ids;
			size_t actualBatchSize = 0;

			vector<size_t>::const_iterator current_sid_start = sid;
			for (size_t batchPosition = 0; batchPosition < batchSize && sid
			    != shard.end(); ++batchPosition) {
				string& input = inputSentences[*sid];
				const vector<string>& refs = referenceSentences[*sid];
				cerr << "Rank " << rank << ", batch position " << batchPosition << endl;
				cerr << "Rank " << rank << ", input sentence " << *sid << ": \"" << input << "\"" << endl;

				vector<ScoreComponentCollection> newFeatureValues;
				vector<float> newBleuScores;
				featureValues.push_back(newFeatureValues);
				bleuScores.push_back(newBleuScores);

				// MODEL
				cerr << "Rank " << rank << ", run decoder to get nbest wrt model score" << endl;
				vector<const Word*> bestModel = decoder->getNBest(input, *sid, n, 0.0,
									1.0, featureValues[batchPosition], bleuScores[batchPosition], true,
									distinctNbest, rank);
				inputLengths.push_back(decoder->getCurrentInputLength());
				ref_ids.push_back(*sid);
				all_ref_ids.push_back(*sid);
				allBestModelScore.push_back(bestModel);
				decoder->cleanup();
				cerr << "Rank " << rank << ", model length: " << bestModel.size() << " Bleu: " << bleuScores[batchPosition][0] << endl;

				// HOPE
				cerr << "Rank " << rank << ", run decoder to get nbest hope translations" << endl;
				size_t oraclePos = featureValues[batchPosition].size();
				oraclePositions.push_back(oraclePos);
				vector<const Word*> oracle = decoder->getNBest(input, *sid, n, 1.0,
									1.0, featureValues[batchPosition], bleuScores[batchPosition], true,
									distinctNbest, rank);
				decoder->cleanup();
				oracles.push_back(oracle);
				cerr << "Rank " << rank << ", oracle length: " << oracle.size() << " Bleu: " << bleuScores[batchPosition][oraclePos] << endl;

				oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
				float oracleBleuScore = bleuScores[batchPosition][oraclePos];
				oracleBleuScores.push_back(oracleBleuScore);

				// FEAR
				cerr << "Rank " << rank << ", run decoder to get nbest fear translations" << endl;
				size_t fearPos = featureValues[batchPosition].size();
				vector<const Word*> fear = decoder->getNBest(input, *sid, n, -1.0, 1.0,
									featureValues[batchPosition], bleuScores[batchPosition], true,
									distinctNbest, rank);
				decoder->cleanup();
				cerr << "Rank " << rank << ", fear length: " << fear.size() << " Bleu: " << bleuScores[batchPosition][fearPos] << endl;

				//			  for (size_t i = 0; i < bestModel.size(); ++i) {
				//					 delete bestModel[i];
				//			  }
				for (size_t i = 0; i < fear.size(); ++i) {
					delete fear[i];
				}

				cerr << "Rank " << rank << ", " << *sid << ", best model Bleu (approximate sentence bleu): "  << bleuScores[batchPosition][0] << endl;
				summedApproxBleu += bleuScores[batchPosition][0];

				// next input sentence
				++sid;
				++actualBatchSize;
				++shardPosition;
			} // end of batch loop

			// Set loss for each sentence as BLEU(oracle) - BLEU(hypothesis)
			vector<vector<float> > losses(actualBatchSize);
			for (size_t batchPosition = 0; batchPosition < actualBatchSize; ++batchPosition) {
				for (size_t j = 0; j < bleuScores[batchPosition].size(); ++j) {
					losses[batchPosition].push_back(oracleBleuScores[batchPosition]
					    - bleuScores[batchPosition][j]);
				}
			}

			// set weight for bleu feature to 0
			const vector<const ScoreProducer*> featureFunctions =
			    StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT).GetFeatureFunctions();
			mosesWeights.Assign(featureFunctions.back(), 0);

			if (!hildreth && typeid(*optimiser) == typeid(MiraOptimiser)) {
				((MiraOptimiser*) optimiser)->setOracleIndices(oraclePositions);
			}

			if (logFeatureValues) {
				for (size_t i = 0; i < featureValues.size(); ++i) {
					for (size_t j = 0; j < featureValues[i].size(); ++j) {
						featureValues[i][j].ApplyLog(baseOfLog);
					}

					oracleFeatureValues[i].ApplyLog(baseOfLog);
				}
			}

			// get 1best model results with old weights
			vector< vector <float > > bestModelOld_batch;
			for (size_t i = 0; i < actualBatchSize; ++i) {
				string& input = inputSentences[*current_sid_start + i];
				vector <float> bestModelOld = decoder->getBleuAndScore(input, *current_sid_start + i, 0.0, distinctNbest);
				bestModelOld_batch.push_back(bestModelOld);
				decoder->cleanup();
			}

			// optionally print out the feature values
			if (print_feature_values) {
				cerr << "\nRank " << rank << ", feature values: " << endl;
				for (size_t i = 0; i < featureValues.size(); ++i) {
					for (size_t j = 0; j < featureValues[i].size(); ++j) {
						cerr << featureValues[i][j] << endl;
					}
				}
				cerr << endl;
			}

			// run optimiser on batch
			cerr << "\nRank " << rank << ", run optimiser:" << endl;
			ScoreComponentCollection oldWeights(mosesWeights);
			vector<int> update_status = optimiser->updateWeights(mosesWeights, featureValues,
			    losses, bleuScores, oracleFeatureValues, oracleBleuScores, ref_ids,
			    learning_rate, max_sentence_update, rank, epoch, updates_per_epoch, controlUpdates);

			if (update_status[0] == 1) {
				cerr << "Rank " << rank << ", no update for batch" << endl;
			}
			else if (update_status[0] == -1) {
				cerr << "Rank " << rank << ", update ignored" << endl;
			}
			else {
				sumConstraintChangeAbs += abs(update_status[1] - update_status[2]);
				sumStillViolatedConstraints += update_status[2];

				if (updates_per_epoch == -1) {
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
						cerr << "Rank " << rank << ", set new average weights: " << mosesWeights << endl;
					}
					else {
						cerr << "Rank " << rank << ", set new weights: " << mosesWeights << endl;
					}

					// set new Moses weights (averaged or not)
					decoder->setWeights(mosesWeights);

					// compute difference to old weights
					ScoreComponentCollection weightDifference(mosesWeights);
					weightDifference.MinusEquals(oldWeights);
					cerr << "Rank " << rank << ", weight difference: " << weightDifference << endl;

					// get 1best model results with new weights (for each sentence in batch)
					vector<float> bestModelNew;
					for (size_t i = 0; i < actualBatchSize; ++i) {
						string& input = inputSentences[*current_sid_start + i];
						bestModelNew = decoder->getBleuAndScore(input, *current_sid_start + i, 0.0, distinctNbest);
						decoder->cleanup();
						cerr << "Rank " << rank << ", epoch " << epoch << ", 1best model bleu, old: " << bestModelOld_batch[i][0] << ", new: " << bestModelNew[0] << endl;
						cerr << "Rank " << rank << ", epoch " << epoch << ", 1best model score, old: " << bestModelOld_batch[i][1] << ", new: " << bestModelNew[1] << endl;
					}
				}
			}

			// update history (for approximate document Bleu)
			for (size_t i = 0; i < oracles.size(); ++i) {
				cerr << "Rank " << rank << ", oracle length: " << oracles[i].size() << " ";
			}
			decoder->updateHistory(oracles, inputLengths, ref_ids, rank, epoch);

			// clean up oracle translations after updating history
			for (size_t i = 0; i < oracles.size(); ++i) {
				for (size_t j = 0; j < oracles[i].size(); ++j) {
					delete oracles[i][j];
				}
			}

			bool makeUpdate = updates_per_epoch == -1 ? 0 : (shardPosition % (shard.size() / updates_per_epoch) == 0);

			// apply accumulated updates
			if (makeUpdate && typeid(*optimiser) == typeid(MiraOptimiser)) {
				mosesWeights = decoder->getWeights();
				ScoreComponentCollection accumulatedUpdates = ((MiraOptimiser*) optimiser)->getAccumulatedUpdates();
				cerr << "\nRank " << rank << ", updates to apply during epoch " << epoch << ": " << accumulatedUpdates << endl;
				if (accumulatedUpdates.GetWeightedScore() != 0) {
					mosesWeights.PlusEquals(accumulatedUpdates);
					((MiraOptimiser*) optimiser)->resetAccumulatedUpdates();

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
						cerr << "Rank " << rank << ", set new average weights after applying cumulative update: " << mosesWeights << endl;
					}
					else {
						cerr << "Rank " << rank << ", set new weights after applying cumulative update: " << mosesWeights << endl;
					}

					decoder->setWeights(mosesWeights);

					// compute difference to old weights
					ScoreComponentCollection weightDifference(mosesWeights);
					weightDifference.MinusEquals(oldWeights);
					cerr << "Rank " << rank << ", weight difference: " << weightDifference << endl;
				}
				else {
					cerr << "Rank " << rank << ", cumulative update is empty.." << endl;
				}
			}

			size_t mixing_base = shard.size() / mixingFrequency;
			size_t dumping_base = shard.size() / weightDumpFrequency;
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

					cerr << "Dumping mixed average weights during epoch " << epoch << " to " << filename.str() << endl;
					mixedAverageWeights.Save(filename.str());
					++weightEpochDump;
				}
			}// end dumping
		} // end of shard loop, end of this epoch

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

		if (stop_optimal) {
			if (epoch > 0) {
				if (sumConstraintChangeAbs_lastEpoch == sumConstraintChangeAbs && sumStillViolatedConstraints_lastEpoch == sumStillViolatedConstraints) {
					cerr << "Rank " << rank << ", epoch " << epoch << ", sum of violated constraints and constraint changes has stayed the same: " << sumStillViolatedConstraints << ", " <<  sumConstraintChangeAbs << endl;
				}
				else {
					cerr << "Rank " << rank << ", epoch " << epoch << ", sum of violated constraints: " << sumStillViolatedConstraints << ", sum of constraint changes " <<  sumConstraintChangeAbs << endl;
				}
			}
			else {
				cerr << "Rank " << rank << ", epoch " << epoch << ", sum of violated constraints: " << sumStillViolatedConstraints << endl;
			}

			sumConstraintChangeAbs_lastEpoch = sumConstraintChangeAbs;
			sumStillViolatedConstraints_lastEpoch = sumStillViolatedConstraints;
		}
		
		if (!stop) {
			if (devBleu) {
				// calculate bleu score of dev set
				vector<float> bleuAndRatio = decoder->calculateBleuOfCorpus(allBestModelScore, all_ref_ids, epoch, rank);
				float bleu = bleuAndRatio[0];
				float ratio = bleuAndRatio[1];

				for (size_t i = 0; i < allBestModelScore.size(); ++i) {
					for (size_t j = 0; j < allBestModelScore[i].size(); ++j) {
						delete allBestModelScore[i][j];
					}
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

				// average ratio across processes
				sendbuf[0] = ratio;
				recvbuf[0] = 0;
				MPI_Reduce(sendbuf, recvbuf, 1, MPI_FLOAT, MPI_SUM, 0, world);
				if (rank == 0) {
					averageRatio = recvbuf[0];

					// divide by number of processes
					averageRatio /= size;
					cerr << "Average ratio (dev) after epoch " << epoch << ": " << averageRatio << endl;
					if (averageRatio > 1.008 && adapt_BPfactor) {
						BPfactor -= 0.05;
						decoder->setBPfactor(BPfactor);
						cerr << "Change BPfactor to " << BPfactor << ".." << endl;
					}
					else if (averageRatio > 1.0 && adapt_BPfactor) {
						BPfactor = 1;
						decoder->setBPfactor(BPfactor);
						cerr << "Change BPfactor to 1.." << endl;
					}
				}

				// average approximate sentence bleu across processes
				sendbuf[0] = summedApproxBleu/numberOfUpdatesThisEpoch;
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
				averageApproxBleu = summedApproxBleu / numberOfUpdatesThisEpoch;
				cerr << "Average approx. sentence Bleu (dev) after epoch " << epoch << ": " << averageApproxBleu << endl;
#endif
				if (rank == 0) {
					if (stop_dev_bleu) {
						if (averageBleu <= prevAverageBleu && prevAverageBleu <= beforePrevAverageBleu) {
							stop = true;
							cerr << "Average Bleu (dev) is decreasing or no more increasing.. stop tuning." << endl;
							ScoreComponentCollection dummy;
							ostringstream endfilename;
							endfilename << "stopping";
							dummy.Save(endfilename.str());
						}
					}

					if (stop_approx_dev_bleu) {
						if (averageApproxBleu <= prevAverageApproxBleu && prevAverageApproxBleu <= beforePrevAverageApproxBleu) {
							stop = true;
							cerr << "Average approx. sentence Bleu (dev) is decreasing or no more increasing.. stop tuning." << endl;
							ScoreComponentCollection dummy;
							ostringstream endfilename;
							endfilename << "stopping";
							dummy.Save(endfilename.str());
						}
					}
				}

#ifdef MPI_ENABLE
				mpi::broadcast(world, stop, 0);
#endif
			} // end if (dev_bleu)

			// Test if weights have converged
			if (weightConvergence) {
				bool reached = true;
				if (rank == 0 && (epoch >= 2)) {
					ScoreComponentCollection firstDiff(mixedAverageWeights);
					firstDiff.MinusEquals(mixedAverageWeightsPrevious);
					cerr << "Average weight changes since previous epoch: " << firstDiff << endl;
					ScoreComponentCollection secondDiff(mixedAverageWeights);
					secondDiff.MinusEquals(mixedAverageWeightsBeforePrevious);
					cerr << "Average weight changes since before previous epoch: " << secondDiff << endl << endl;

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
						cerr << "Stopping criterion has been reached after epoch " << epoch << ".. stopping MIRA." << endl;
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

			// if using flexible margin scale factor, increase scaling (decrease value) for next epoch
			if (marginScaleFactorStep > 0) {
				if (marginScaleFactor - marginScaleFactorStep >= marginScaleFactorMin) {
					if (typeid(*optimiser) == typeid(MiraOptimiser)) {
								marginScaleFactor -= marginScaleFactorStep;
								cerr << "Change margin scale factor to: " << marginScaleFactor << endl;
								((MiraOptimiser*) optimiser)->setMarginScaleFactor(marginScaleFactor);
					}
				}
			}

			// if using flexible slack, increase slack for next epoch
			if (slack_step > 0) {
				if (slack + slack_step <= slack_max) {
					if (typeid(*optimiser) == typeid(MiraOptimiser)) {
						slack += slack_step;
						cerr << "Change slack to: " << slack << endl;
						((MiraOptimiser*) optimiser)->setSlack(slack);
					}
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
				cerr << "Change learning rate to " << learning_rate << endl;
			}

			// change maximum sentence update
			if ((decrease_sentence_update > 0) && (max_sentence_update - decrease_sentence_update >= min_sentence_update)) {
				max_sentence_update -= decrease_sentence_update;
				if (max_sentence_update <= 0.0001) {
					max_sentence_update = 0;
					stop = true;
#ifdef MPI_ENABLE
					mpi::broadcast(world, stop, 0);
#endif
				}
				cerr << "Change maximum sentence update to " << max_sentence_update << endl;
			}
		}
	} // end of epoch loop

#ifdef MPI_ENABLE
	MPI_Finalize();
#endif

	now = time(0); // get current time
	tm = localtime(&now); // get struct filled out
	cerr << "\nEnd date/time: " << tm->tm_mon + 1 << "/" << tm->tm_mday
			<< "/" << tm->tm_year + 1900 << ", " << tm->tm_hour << ":"
			<< tm->tm_min << ":" << tm->tm_sec << endl;

	delete decoder;
	exit(0);
}

