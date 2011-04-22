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
#include "Hildreth.h"

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

struct RandomIndex {
	ptrdiff_t operator()(ptrdiff_t max) {
		srand(time(0));  // Initialize random number generator with current time.
		return static_cast<ptrdiff_t> (rand() % max);
	}
};

void shuffleInput(vector<size_t>& order, size_t size, size_t inputSize) {
	cerr << "Shuffling input examples.." << endl;
//	RandomIndex rindex;
//	random_shuffle(order.begin(), order.end(), rindex);

	// remove first element and put it in the back
	size_t first = order.at(0);
	size_t index = 0;
	order.erase(order.begin());
	order.push_back(first);
}

void createShard(vector<size_t>& order, size_t size, size_t rank, vector<size_t>& shard) {
	// Create the shards according to the number of processes used
	float shardSize = (float) (order.size()) / size;
	size_t shardStart = (size_t) (shardSize * rank);
	size_t shardEnd = (size_t) (shardSize * (rank + 1));
	if (rank == size - 1)
		shardEnd = order.size();
	shard.resize(shardSize);
	copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());
	cerr << "order: ";
	for (size_t i = 0; i < shard.size(); ++i) {
		cerr << shard[i] << " ";
	}
	cerr << endl;
}

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
	bool averageWeights;
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
	bool one_constraint;
	bool one_per_batch;
	bool print_feature_values;
	bool stop_dev_bleu;
	bool stop_approx_dev_bleu;
	bool stop_optimal;
	bool train_linear_classifier;
	int updates_per_epoch;
	bool multiplyA;
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
	  ("multiplyA", po::value<bool>(&multiplyA)->default_value(true), "Multiply A with outcome before passing to Hildreth")
	  ("nbest,n", po::value<size_t>(&n)->default_value(10), "Number of translations in nbest list")
			("normalise", po::value<bool>(&normaliseWeights)->default_value(false), "Whether to normalise the updated weights before passing them to the decoder")
			("one-constraint", po::value<bool>(&one_constraint)->default_value(false), "Forget about hope and fear and consider only the 1best model translation to formulate a constraint")
	  ("one-per-batch", po::value<bool>(&one_per_batch)->default_value(false), "Only 1 constraint per batch for params --accumulate-most-violated.. and --past-and-current..")
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
			("train-linear-classifier", po::value<bool>(&train_linear_classifier)->default_value(false), "Test algorithm for linear classification")
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

	if (train_linear_classifier) {
		FName name_x("x");
		FName name_y("y");
		FVector weights;
		weights.set(name_x, 0);
		weights.set(name_y, 0);

		vector<FVector> examples;
		vector<int> outcomes;

		FVector pos1;
		pos1.set(name_x, 1);
		pos1.set(name_y, 1);
		FVector pos2;
		pos2.set(name_x, 2);
		pos2.set(name_y, 1);
		FVector pos3;
		pos3.set(name_x, 3);
		pos3.set(name_y, 1);
		FVector pos4;
		pos4.set(name_x, 8);
		pos4.set(name_y, 1);
		FVector neg1;
		neg1.set(name_x, 1);
		neg1.set(name_y, -1);
		FVector neg2;
		neg2.set(name_x, 2);
		neg2.set(name_y, -1);
		FVector neg3;
		neg3.set(name_x, 3);
		neg3.set(name_y, -1);
		FVector neg4;
		neg4.set(name_x, 8);
		neg4.set(name_y, -1);
		examples.push_back(pos1);
		examples.push_back(neg1);
		examples.push_back(pos2);
		examples.push_back(neg2);
		examples.push_back(pos3);
		examples.push_back(neg3);
		examples.push_back(pos4);
		examples.push_back(neg4);
		outcomes.push_back(1);
		outcomes.push_back(-1);
		outcomes.push_back(1);
		outcomes.push_back(-1);
		outcomes.push_back(1);
		outcomes.push_back(-1);
		outcomes.push_back(1);
		outcomes.push_back(-1);

		// add outlier
		FVector pos5;
		pos5.set(name_x, 2.5);
		pos5.set(name_y, -1.5);
		examples.push_back(pos5);
		outcomes.push_back(1);

		FVector neg5;
		neg5.set(name_x, 0);
		neg5.set(name_y, -1);
		examples.push_back(neg5);
		outcomes.push_back(-1);
		
		// create order
		vector<size_t> order;
		if (rank == 0) {
			for (size_t i = 0; i < examples.size(); ++i) {
				order.push_back(i);
			}
		}

		cerr << "weights: " << weights << endl;
		cerr << "slack: " << slack << endl;
		bool stop = false;
		FVector prevFinalWeights;
		FVector prevPrevFinalWeights;
		//		float epsilon = 0.0001;
		float epsilon = 0.001;
		for (size_t epoch = 0; !stop && epoch < epochs; ++epoch) {
			cerr << "\nEpoch " << epoch << endl;
			size_t updatesThisEpoch = 0;

			// optionally shuffle the input data --> change order
			if (shuffle && rank == 0) {
				shuffleInput(order, size, examples.size());
			}

			// create shard
			vector<size_t> shard;
			createShard(order, size, rank, shard);

			cerr << "\nBefore: y * x_i . w >= 1 ?" << endl;
			size_t numberUnsatisfiedConstraintsBefore = 0;
			float errorSumBefore = 0;
			// outcome_i (x_i . w + b) >= +1  (b = 0 in this example)
			for (size_t i = 0; i < examples.size(); ++i) {
				float innerProduct = examples[i].inner_product(weights);
				float leftHandSide = outcomes[i] * innerProduct;
				cerr << outcomes[i] << " * " << innerProduct << " >= " << 1 << "? " << (leftHandSide >= 1) << " (error: " << 1 - leftHandSide << ")" << endl;
				float diff = 1 - leftHandSide;
				if (diff > epsilon) {
					++numberUnsatisfiedConstraintsBefore;
					errorSumBefore += 1 - leftHandSide;
				}
			}
			cerr << "unsatisfied constraints before: " << numberUnsatisfiedConstraintsBefore << endl;
			cerr << "error sum before: " << errorSumBefore << endl;

			// iterate over training data
			size_t shardPosition = 0;
			vector<size_t>::const_iterator sid = shard.begin();

			// Use updatedWeights during one epoch
			FVector updatedWeights(weights);
			while (sid != shard.end()) {
				vector<FVector> A;
				vector<FVector> A_mult;
				vector<float> b;
				vector<float> batch_outcomes;

				// collect constraints for batch
				size_t actualBatchSize = 0;
				for (size_t batchPosition = 0; batchPosition < batchSize && sid != shard.end(); ++batchPosition) {
				        A.push_back(examples[*sid]);
					A_mult.push_back(outcomes[*sid]* examples[*sid]);
					b.push_back(1);
					batch_outcomes.push_back(outcomes[*sid]);

					// next input sentence
					++sid;
					++actualBatchSize;
					++shardPosition;
				}

				// check if constraints are already satisfied for batch
				size_t batch_numberUnsatisfiedConstraintsBefore = 0;
				float batch_errorSumBefore = 0;
				for (size_t i = 0; i < A.size(); ++i) {
					float innerProduct = A[i].inner_product(updatedWeights);
					float leftHandSide = batch_outcomes[i] * innerProduct;
					cerr << batch_outcomes[i] << " * " << innerProduct << " >= " << 1 << "? " << (leftHandSide >= 1) << " (error: " << 1 - leftHandSide << ")" << endl;
					float diff = 1 - leftHandSide;
					if (diff > epsilon) {
						++batch_numberUnsatisfiedConstraintsBefore;
						batch_errorSumBefore += 1 - leftHandSide;
					}
				}
				cerr << "batch: unsatisfied constraints before: " << batch_numberUnsatisfiedConstraintsBefore << endl;
				cerr << "batch: error sum before: " << batch_errorSumBefore << endl;

				if (batch_numberUnsatisfiedConstraintsBefore != 0) {
					// pass constraints to optimizer
					for (size_t j = 0; j < A.size(); ++j){
					  if (multiplyA) {
					    cerr << "A: " << A_mult[j] << endl;
					  }
					  else {
					    cerr << "A: " << A[j] << endl;
					  }
					}

					vector<float> alphas;
					if (slack == 0) {
					  if (multiplyA) {
					    alphas = Hildreth::optimise(A_mult, b);
					  }
					  else {
					    alphas = Hildreth::optimise(A, b);
					  }
					}
					else {
					  if (multiplyA) {
					    alphas = Hildreth::optimise(A_mult, b, slack);
					  }
					  else{
					    alphas = Hildreth::optimise(A, b, slack);
					  }
					}
					
					for (size_t j = 0; j < alphas.size(); ++j) {
						cerr << "alpha:" << alphas[j] << endl;
						updatedWeights += batch_outcomes[j] * A[j] * alphas[j];						
					}
					cerr << "potential new weights: " << updatedWeights << endl;

					// check if constraints are satisfied after processing batch
					size_t batch_numberUnsatisfiedConstraintsAfter = 0;
					float batch_errorSumAfter = 0;
					for (size_t i = 0; i < A.size(); ++i) {
						float innerProduct = A[i].inner_product(updatedWeights);
						float leftHandSide = batch_outcomes[i] * innerProduct;
						cerr << batch_outcomes[i] << " * " << innerProduct << " >= " << 1 << "? " << (leftHandSide >= 1) << " (error: " << 1 - leftHandSide << ")" << endl;
						float diff = 1 - leftHandSide;
						if (diff > epsilon) {
							++batch_numberUnsatisfiedConstraintsAfter;
							batch_errorSumAfter += 1 - leftHandSide;
						}
					}
					cerr << "batch: unsatisfied constraints after: " << batch_numberUnsatisfiedConstraintsAfter << endl;
					cerr << "batch: error sum after: " << batch_errorSumAfter << endl << endl;
				}
				else {
					cerr << "batch: all constraints satisfied." << endl;
				}
			} // end of epoch */

			cerr << "After: y * x_i . w >= 1 ?" << endl;
			size_t numberUnsatisfiedConstraintsAfter = 0;

			float errorSumAfter = 0;
			for (size_t i = 0; i < examples.size(); ++i) {
				float innerProduct = examples[i].inner_product(updatedWeights);
				float leftHandSide = outcomes[i] * innerProduct;
				cerr << outcomes[i] << " * " << innerProduct << " >= " << 1 << "? " << (leftHandSide >= 1) << " (error: " << 1 - leftHandSide << ")" << endl;
				float diff = 1 - leftHandSide;
				if (diff > epsilon) {
					++numberUnsatisfiedConstraintsAfter;
					errorSumAfter += 1 - leftHandSide;
				}
			}
			cerr << "unsatisfied constraints after: " << numberUnsatisfiedConstraintsAfter << endl;
			cerr << "error sum after: " << errorSumAfter << endl;

			float epsilon = 0.0001;
			float diff = errorSumAfter - errorSumBefore;
			if (numberUnsatisfiedConstraintsAfter == 0) {
				weights = updatedWeights;
				cerr << "All constraints satisfied during this epoch, stop." << endl;
				cerr << "new weights: " << weights << endl;
				++updatesThisEpoch;
				stop = true;
			}
			else if (numberUnsatisfiedConstraintsAfter < numberUnsatisfiedConstraintsBefore) {
				cerr << "Constraints improved during this epoch." << endl;
				weights = updatedWeights;
				cerr << "new weights: " << weights << endl;
				++updatesThisEpoch;
			}
			else if (numberUnsatisfiedConstraintsAfter == numberUnsatisfiedConstraintsBefore && errorSumAfter < errorSumBefore) {
				cerr << "Error improved during this epoch." << endl;
				weights = updatedWeights;
				cerr << "new weights: " << weights << endl;
				++updatesThisEpoch;
			}
			else if(numberUnsatisfiedConstraintsAfter == numberUnsatisfiedConstraintsBefore && (diff < epsilon && diff > epsilon * -1)) {
				cerr << "No changes to constraints or error during this epoch." << endl;
			}
			else {
				cerr << "Constraints/error got worse during this epoch." << endl;
			}

			if (updatesThisEpoch == 0) {
				stop = true;
				cerr << "No more updates, stop." << endl;
			}
			else if(prevFinalWeights == weights) {
				stop = true;
				cerr << "Final weights not changing anymore, stop." << endl;
			}
			else if(prevPrevFinalWeights == weights) {
				stop = true;
				cerr << "Final weights changing back to previous final weights, take average and stop." << endl;
				weights = prevFinalWeights;
				weights += prevPrevFinalWeights;
				weights /= 2;
			}

			prevPrevFinalWeights = prevFinalWeights;
			prevFinalWeights = weights;
		}

		cerr << "\nFinal: " << endl;
		cerr << weights << endl;

		// classify new examples
		cerr << "\nTest examples:" << endl;
		FVector test_pos1;
		test_pos1.set(name_x, 7);
		test_pos1.set(name_y, 1);
		cerr << "pos1 (7, 1): " << test_pos1.inner_product(weights) << endl;
		FVector test_pos2;
		test_pos2.set(name_x, 6);
		test_pos2.set(name_y, 2);
		cerr << "pos2 (2, 2): " << test_pos2.inner_product(weights) << endl;
/*		FVector test_pos3;
		test_pos3.set(name_x, 1);
		test_pos3.set(name_y, 0.5);
		cerr << "pos3 (1, 0.5): " << test_pos3.inner_product(weights) << endl;*/
		FVector test_neg1;
		test_neg1.set(name_x, 7);
		test_neg1.set(name_y, -1);
		cerr << "neg1 (7, -1): " << test_neg1.inner_product(weights) << endl;
		FVector test_neg2;
		test_neg2.set(name_x, 6);
		test_neg2.set(name_y, -2);
		cerr << "neg2 (2, -2): " << test_neg2.inner_product(weights) << endl;
/*		FVector test_neg3;
		test_neg3.set(name_x, 1);
		test_neg3.set(name_y, -0.5);
		cerr << "neg3 (1, -0.5): " << test_neg3.inner_product(weights) << endl;*/

		exit(0);
	}

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

	if (accumulateMostViolatedConstraints && pastAndCurrentConstraints) {
	  cerr << "Error: the parameters --accumulate-most-violated-constraints and --past-and-current-constraints are mutually exclusive" << endl;
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
					      accumulateMostViolatedConstraints, pastAndCurrentConstraints, one_per_batch,
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

				if (one_constraint) {
					cerr << "Rank " << rank << ", run decoder to get 1best wrt model score" << endl;
					vector<const Word*> bestModel = decoder->getNBest(input, *sid, 1, 0.0,
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
					vector<const Word*> oracle = decoder->getNBest(input, *sid, 1, 1.0,
							1.0, featureValues[batchPosition], bleuScores[batchPosition], true,
							distinctNbest, rank);
					decoder->cleanup();
					oracles.push_back(oracle);
					cerr << "Rank " << rank << ", oracle length: " << oracle.size() << " Bleu: " << bleuScores[batchPosition][oraclePos] << endl;

					oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
					float oracleBleuScore = bleuScores[batchPosition][oraclePos];
					oracleBleuScores.push_back(oracleBleuScore);
				}
				else {
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
				}

				cerr << "Rank " << rank << ", sentence " << *sid << ", best model Bleu (approximate sentence bleu): "  << bleuScores[batchPosition][0] << endl;
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
			vector<int> update_status;
			if (one_constraint) {
				update_status = optimiser->updateWeightsAnalytically(mosesWeights, featureValues[0][0],
							    losses[0][0], oracleFeatureValues[0], oracleBleuScores[0], ref_ids[0],
							    learning_rate, max_sentence_update, rank, epoch, controlUpdates);
			}
			else {
			 update_status = optimiser->updateWeights(mosesWeights, featureValues,
			    losses, bleuScores, oracleFeatureValues, oracleBleuScores, ref_ids,
			    learning_rate, max_sentence_update, rank, epoch, updates_per_epoch, controlUpdates);
			}

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

