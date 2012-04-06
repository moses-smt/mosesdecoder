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

	bool help;
	int verbosity;
	string mosesConfigFile;
	string inputFile;
	vector<string> referenceFiles;
	vector<string> mosesConfigFilesFolds, inputFilesFolds, referenceFilesFolds;
	string coreWeightFile;
	size_t epochs;
	string learner;
	bool shuffle;
	size_t mixingFrequency;
	size_t weightDumpFrequency;
	string weightDumpStem;
	float min_learning_rate;
	bool scale_margin, scale_margin_precision;
	bool scale_update, scale_update_precision;
	size_t n;
	size_t batchSize;
	bool distinctNbest;
	bool onlyViolatedConstraints;
	bool accumulateWeights;
	float historySmoothing;
	bool scaleByInputLength, scaleByAvgInputLength;
	bool scaleByInverseLength, scaleByAvgInverseLength;
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
	float bleuWeight, bleuWeight_hope, bleuWeight_fear;
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
	float min_oracle_bleu;
	float minBleuRatio, maxBleuRatio;
	bool boost;
	bool decode_hope, decode_fear, decode_model;
	string decode_filename;
	size_t update_scheme;
	bool separateUpdates, batchEqualsShard;
	bool sparseAverage, dumpMixedWeights, sparseNoAverage;
	bool useSourceLengthHistory;
	int featureCutoff;
	bool pruneZeroWeights;
	bool megam;
	bool printFeatureCounts, printNbestWithFeatures;
	bool avgRefLength;
	po::options_description desc("Allowed options");
	desc.add_options()
		("slack", po::value<float>(&slack)->default_value(0.01), "Use slack in optimiser")
		("dummy", po::value<float>(&dummy)->default_value(-1), "Dummy variable for slack")
		("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
		("adapt-after-epoch", po::value<size_t>(&adapt_after_epoch)->default_value(0), "Index of epoch after which adaptive parameters will be adapted")
		("average-weights", po::value<bool>(&averageWeights)->default_value(false), "Set decoder weights to average weights after each update")
		("avg-ref-length", po::value<bool>(&avgRefLength)->default_value(false), "Use average reference length instead of shortest for BLEU score feature")
		("base-of-log", po::value<size_t>(&baseOfLog)->default_value(10), "Base for taking logs of feature values")
		("batch-equals-shard", po::value<bool>(&batchEqualsShard)->default_value(false), "Batch size is equal to shard size (purely batch)")
		("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
		("bleu-score-weight", po::value<float>(&bleuWeight)->default_value(1.0), "Bleu score weight used in the decoder objective function (on top of the Bleu objective weight)")
		("bleu-score-weight-hope", po::value<float>(&bleuWeight_hope)->default_value(-1), "Bleu score weight used in the decoder objective function for hope translations")
		("bleu-score-weight-fear", po::value<float>(&bleuWeight_fear)->default_value(-1), "Bleu score weight used in the decoder objective function for fear translations")
		("bleu-smoothing-scheme", po::value<size_t>(&bleu_smoothing_scheme)->default_value(1), "Set a smoothing scheme for sentence-Bleu: +1 (1), +0.1 (2), papineni (3) (default:1)")
		("boost", po::value<bool>(&boost)->default_value(false), "Apply boosting factor to updates on misranked candidates")
		("config,f", po::value<string>(&mosesConfigFile), "Moses ini-file")
		("configs-folds", po::value<vector<string> >(&mosesConfigFilesFolds), "Moses ini-files, one for each fold")
		("core-weights", po::value<string>(&coreWeightFile), "Weight file containing the core weights (already tuned, have to be non-zero)")
		("decode-hope", po::value<bool>(&decode_hope)->default_value(false), "Decode dev input set according to hope objective")
		("decode-fear", po::value<bool>(&decode_fear)->default_value(false), "Decode dev input set according to fear objective")
		("decode-model", po::value<bool>(&decode_model)->default_value(false), "Decode dev input set according to normal objective")
		("decode-filename", po::value<string>(&decode_filename), "Filename for Bleu objective translations")
		("decoder-settings", po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
		("decr-learning-rate", po::value<float>(&decrease_learning_rate)->default_value(0),"Decrease learning rate by the given value after every epoch")
		("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(true), "Use n-best list with distinct translations in inference step")
		("dump-mixed-weights", po::value<bool>(&dumpMixedWeights)->default_value(false), "Dump mixed weights instead of averaged weights")
		("epochs,e", po::value<size_t>(&epochs)->default_value(10), "Number of epochs")
		("feature-cutoff", po::value<int>(&featureCutoff)->default_value(-1), "Feature cutoff as additional regularization for sparse features")
		("fear-n", po::value<int>(&fear_n)->default_value(-1), "Number of fear translations used")
		("help", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
		("history-of-1best", po::value<bool>(&historyOf1best)->default_value(false), "Use 1best translations to update the history")
		("history-of-oracles", po::value<bool>(&historyOfOracles)->default_value(false), "Use oracle translations to update the history")
		("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.9), "Adjust the factor for history smoothing")
		("hope-fear", po::value<bool>(&hope_fear)->default_value(true), "Use only hope and fear translations for optimisation (not model)")
		("hope-fear-rank", po::value<bool>(&hope_fear_rank)->default_value(false), "Use hope and fear translations for optimisation, use model for ranking")
		("hope-model", po::value<bool>(&hope_model)->default_value(false), "Use only hope and model translations for optimisation (use --fear-n to set number of model translations)")
		("hope-n", po::value<int>(&hope_n)->default_value(-1), "Number of hope translations used")
		("input-file,i", po::value<string>(&inputFile), "Input file containing tokenised source")
		("input-files-folds", po::value<vector<string> >(&inputFilesFolds), "Input files containing tokenised source, one for each fold")
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
		("megam", po::value<bool>(&megam)->default_value(false), "Use megam for optimization step")
		("min-learning-rate", po::value<float>(&min_learning_rate)->default_value(0), "Set a minimum learning rate")
		("min-oracle-bleu", po::value<float>(&min_oracle_bleu)->default_value(0), "Set a minimum oracle BLEU score")
		("min-weight-change", po::value<float>(&min_weight_change)->default_value(0.01), "Set minimum weight change for stopping criterion")
		("mira-learning-rate", po::value<float>(&mira_learning_rate)->default_value(1), "Learning rate for MIRA (fixed or flexible)")
		("mixing-frequency", po::value<size_t>(&mixingFrequency)->default_value(1), "How often per epoch to mix weights, when using mpi")
		("model-hope-fear", po::value<bool>(&model_hope_fear)->default_value(false), "Use model, hope and fear translations for optimisation")
		("nbest,n", po::value<size_t>(&n)->default_value(1), "Number of translations in n-best list")
		("normalise", po::value<bool>(&normaliseWeights)->default_value(false), "Whether to normalise the updated weights before passing them to the decoder")
		("normalise-margin", po::value<bool>(&normaliseMargin)->default_value(false), "Normalise the margin: squash between 0 and 1")
		("only-violated-constraints", po::value<bool>(&onlyViolatedConstraints)->default_value(false), "Add only violated constraints to the optimisation problem")
		("perceptron-learning-rate", po::value<float>(&perceptron_learning_rate)->default_value(0.01), "Perceptron learning rate")
		("print-feature-values", po::value<bool>(&print_feature_values)->default_value(false), "Print out feature values")
		("print-feature-counts", po::value<bool>(&printFeatureCounts)->default_value(false), "Print out feature values, print feature list with hope counts after 1st epoch")
		("print-nbest-with-features", po::value<bool>(&printNbestWithFeatures)->default_value(false), "Print out feature values, print feature list with hope counts after 1st epoch")
		("prune-zero-weights", po::value<bool>(&pruneZeroWeights)->default_value(false), "Prune zero-valued sparse feature weights")				
		("rank-n", po::value<int>(&rank_n)->default_value(-1), "Number of translations used for ranking")
		("rank-only", po::value<bool>(&rank_only)->default_value(false), "Use only model translations for optimisation")
		("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training")
		("reference-files-folds", po::value<vector<string> >(&referenceFilesFolds), "Reference translation files for training, one for each fold")		
		("relax-BP", po::value<float>(&relax_BP)->default_value(1), "Relax the BP by setting this value between 0 and 1")
		("scale-by-inverse-length", po::value<bool>(&scaleByInverseLength)->default_value(false), "Scale the BLEU score by (a history of) the inverse input length")
		("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale the BLEU score by (a history of) the input length")
		("scale-by-avg-input-length", po::value<bool>(&scaleByAvgInputLength)->default_value(false), "Scale BLEU by an average of the input length")
		("scale-by-avg-inverse-length", po::value<bool>(&scaleByAvgInverseLength)->default_value(false), "Scale BLEU by an average of the inverse input length")
		("scale-by-x", po::value<float>(&scaleByX)->default_value(1), "Scale the BLEU score by value x")
		("scale-margin", po::value<bool>(&scale_margin)->default_value(0), "Scale the margin by the Bleu score of the oracle translation")
		("scale-margin-precision", po::value<bool>(&scale_margin_precision)->default_value(0), "Scale the margin by the precision of the oracle translation")
		("scale-update", po::value<bool>(&scale_update)->default_value(0), "Scale the update by the Bleu score of the oracle translation")       
		("scale-update-precision", po::value<bool>(&scale_update_precision)->default_value(0), "Scale the update by the precision of the oracle translation")	
		("sentence-level-bleu", po::value<bool>(&sentenceLevelBleu)->default_value(true), "Use a sentences level Bleu scoring function")
		("separate-updates", po::value<bool>(&separateUpdates)->default_value(false), "Compute separate updates for each sentence in a batch")
		("shuffle", po::value<bool>(&shuffle)->default_value(false), "Shuffle input sentences before processing")
		("slack-min", po::value<float>(&slack_min)->default_value(0.01), "Minimum slack used")
		("slack-step", po::value<float>(&slack_step)->default_value(0), "Increase slack from epoch to epoch by the value provided")
		("sparse-average", po::value<bool>(&sparseAverage)->default_value(false), "Average weights by the number of processes")
		("sparse-no-average", po::value<bool>(&sparseNoAverage)->default_value(false), "Don't average sparse weights, just sum")
		("stop-weights", po::value<bool>(&weightConvergence)->default_value(true), "Stop when weights converge")
		("threads", po::value<int>(&threadcount)->default_value(1), "Number of threads used")
		("update-scheme", po::value<size_t>(&update_scheme)->default_value(1), "Update scheme, default: 1")
		("use-source-length-history", po::value<bool>(&useSourceLengthHistory)->default_value(false), "Use history of source length instead of target length for history Bleu")
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

	const StaticData &staticData = StaticData::Instance();

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

  bool trainWithMultipleFolds = false; 
  if (mosesConfigFilesFolds.size() > 0 || inputFilesFolds.size() > 0 || referenceFilesFolds.size() > 0) {
  	if (rank == 0)
  		cerr << "Training with " << mosesConfigFilesFolds.size() << " folds" << endl;
  	trainWithMultipleFolds = true;
  }
  
  if (dummy != -1)
    slack = dummy;

  if (dumpMixedWeights && (mixingFrequency != weightDumpFrequency)) {
	  cerr << "Set mixing frequency = weight dump frequency for dumping mixed weights!" << endl;
	  exit(1);
  }

  if ((sparseAverage || sparseNoAverage) && averageWeights) {
	  cerr << "Parameters --sparse-average 1/--sparse-no-average 1 and --average-weights 1 are incompatible (not implemented)" << endl;
	  exit(1);
  }

  if (trainWithMultipleFolds) {
	  if (!mosesConfigFilesFolds.size()) {
		  cerr << "Error: No moses ini files specified for training with folds" << endl;
		  exit(1);
	  }
	  
	  if (!inputFilesFolds.size()) {
		  cerr << "Error: No input files specified for training with folds" << endl;
		  exit(1);
	  }

	  if (!referenceFilesFolds.size()) {
		  cerr << "Error: No reference files specified for training with folds" << endl;
		  exit(1);
	  }
  }
  else {
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
  }

	// load input and references
  	vector<string> inputSentences;
  	size_t inputSize = trainWithMultipleFolds? inputFilesFolds.size(): 0;
  	size_t refSize = trainWithMultipleFolds? referenceFilesFolds.size(): referenceFiles.size(); 
  	vector<vector<string> > inputSentencesFolds(inputSize);
  	vector<vector<string> > referenceSentences(refSize);
  	
	// number of cores for each fold
  	size_t coresPerFold = 0, myFold = 0;
  	if (trainWithMultipleFolds) {
  		if (mosesConfigFilesFolds.size() > size) {
  			cerr << "Number of cores has to be a multiple of the number of folds" << endl;
  			exit(1);
  		}
  		coresPerFold = size/mosesConfigFilesFolds.size();
  		if (size % coresPerFold > 0) {
  			cerr << "Number of cores has to be a multiple of the number of folds" << endl;
  			exit(1);
  		} 
		
  		if (rank == 0)
  			cerr << "Number of cores per fold: " << coresPerFold << endl;		
  		myFold = rank/coresPerFold;
  		cerr << "Rank " << rank << ", my fold: " << myFold << endl;
  	}
  	
  	// NOTE: we do not actually need the references here, because we are reading them in from StaticData
  	if (trainWithMultipleFolds) {
  		if (!loadSentences(inputFilesFolds[myFold], inputSentencesFolds[myFold])) {
  			cerr << "Error: Failed to load input sentences from " << inputFilesFolds[myFold] << endl;
  			exit(1);
  		}
  		VERBOSE(1, "Rank " << rank << " reading inputs from " << inputFilesFolds[myFold] << endl);
  		
  		if (!loadSentences(referenceFilesFolds[myFold], referenceSentences[myFold])) {
  			cerr << "Error: Failed to load reference sentences from " << referenceFilesFolds[myFold] << endl;
  			exit(1);
  		}
  		if (referenceSentences[myFold].size() != inputSentencesFolds[myFold].size()) {
  			cerr << "Error: Input file length (" << inputSentencesFolds[myFold].size() << ") != ("
  				<< referenceSentences[myFold].size() << ") reference file length (rank " << rank << ")" << endl;
  			exit(1);
  		}
  		VERBOSE(1, "Rank " << rank << " reading references from " << referenceFilesFolds[myFold] << endl);  		
  	}
  	else {
  		if (!loadSentences(inputFile, inputSentences)) {
  			cerr << "Error: Failed to load input sentences from " << inputFile << endl;
  			return 1;
  		}
  		
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
  	}

	if (scaleByAvgInputLength ||  scaleByInverseLength || scaleByAvgInverseLength)
		scaleByInputLength = false;

	if (historyOf1best || historyOfOracles)
		sentenceLevelBleu = false;
	if (!sentenceLevelBleu) {
		if (!historyOf1best && !historyOfOracles) {
			historyOf1best = true;
		}
	}

	// initialise Moses
	// add initial Bleu weight and references to initialize Bleu feature
	boost::trim(decoder_settings);
	decoder_settings += " -weight-bl 1 -references";
	if (trainWithMultipleFolds) {
		decoder_settings += " ";
		decoder_settings += referenceFilesFolds[myFold];
	}
	else {
		for (size_t i=0; i < referenceFiles.size(); ++i) {
			decoder_settings += " ";
			decoder_settings += referenceFiles[i];
		}
	}
	
	vector<string> decoder_params;
	boost::split(decoder_params, decoder_settings, boost::is_any_of("\t "));
	
	string configFile = trainWithMultipleFolds? mosesConfigFilesFolds[myFold] : mosesConfigFile;
	VERBOSE(1, "Rank " << rank << " reading config file from " << configFile << endl);
	MosesDecoder* decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
	decoder->setBleuParameters(sentenceLevelBleu, scaleByInputLength, scaleByAvgInputLength,
			scaleByInverseLength, scaleByAvgInverseLength,
			scaleByX, historySmoothing, bleu_smoothing_scheme, relax_BP, useSourceLengthHistory);
	SearchAlgorithm searchAlgorithm = staticData.GetSearchAlgorithm();
	bool chartDecoding = (searchAlgorithm == ChartDecoding);

	if (normaliseWeights) {
		ScoreComponentCollection startWeights = decoder->getWeights();
		startWeights.L1Normalise();
		decoder->setWeights(startWeights);
	}

	if (decode_hope || decode_fear || decode_model) {
		size_t decode = 1;
		if (decode_fear) decode = 2;
		if (decode_model) decode = 3;
		decodeHopeOrFear(rank, size, decode, decode_filename, inputSentences, decoder, n);
	}

	// Optionally shuffle the sentences
	vector<size_t> order;
	if (trainWithMultipleFolds) {  	
		for (size_t i = 0; i < inputSentencesFolds[myFold].size(); ++i) {
			order.push_back(i);
		}
		
		if (shuffle) {
			cerr << "Shuffling input sentences.." << endl;
			RandomIndex rindex;
			random_shuffle(order.begin(), order.end(), rindex);
		}
	}
	else {
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
	}

	// initialise optimizer
	Optimiser* optimiser = NULL;
	if (learner == "mira") {
		if (rank == 0) {
			cerr << "Optimising using Mira" << endl;
			cerr << "slack: " << slack << ", learning rate: " << mira_learning_rate << endl;
		}
		optimiser = new MiraOptimiser(onlyViolatedConstraints, slack, scale_margin, scale_margin_precision,
				scale_update, scale_update_precision, margin_slack, boost, update_scheme, normaliseMargin);
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

	if (model_hope_fear || hope_model || rank_only || hope_fear_rank || megam)
		hope_fear = false; // is true by default
	if (learner == "mira" && !(hope_fear || hope_model || model_hope_fear || rank_only || hope_fear_rank || megam)) {
		cerr << "Error: Need to select one of parameters --hope-fear/--model-hope-fear for mira update." << endl;
		return 1;
	}

	if (bleuWeight_hope == -1) {
		bleuWeight_hope = bleuWeight;
	}
	if (bleuWeight_fear == -1) {
		bleuWeight_fear = bleuWeight;
	}

	if (max_length_dev_all != -1) {
		max_length_dev_hypos = max_length_dev_all;
		max_length_dev_hope_ref = max_length_dev_all;
		max_length_dev_fear_ref = max_length_dev_all;
	}

#ifdef MPI_ENABLE
	if (!trainWithMultipleFolds)
	  mpi::broadcast(world, order, 0);
#endif

	// Create shards according to the number of processes used
	vector<size_t> shard;
	if (trainWithMultipleFolds) {			
		float shardSize = (float) (order.size())/coresPerFold;
		size_t shardStart = (size_t) (shardSize * (rank % coresPerFold));
		size_t shardEnd = shardStart + shardSize;
		if (rank % coresPerFold == coresPerFold - 1) { // last rank of each fold 
			shardEnd = order.size();
			shardSize = shardEnd - shardStart;
		}		
		VERBOSE(1, "Rank: " << rank << ", shard size: " << shardSize << endl);
		VERBOSE(1, "Rank: " << rank << ", shard start: " << shardStart << " shard end: " << shardEnd << endl);
		shard.resize(shardSize);
		copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());
		batchSize = 1;
	}
	else {
		float shardSize = (float) (order.size()) / size;
		size_t shardStart = (size_t) (shardSize * rank);
		size_t shardEnd = (size_t) (shardSize * (rank + 1));
		if (rank == size - 1) {
			shardEnd = order.size();
			shardSize = shardEnd - shardStart;
		}
		VERBOSE(1, "Shard size: " << shardSize << endl);
		VERBOSE(1, "Rank: " << rank << " Shard start: " << shardStart << " Shard end: " << shardEnd << endl);
		shard.resize(shardSize);
		copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());
		if (batchEqualsShard)
			batchSize = shardSize;
	}

	// get reference to feature functions
	const vector<const ScoreProducer*> featureFunctions =
			staticData.GetTranslationSystem(TranslationSystem::DEFAULT).GetFeatureFunctions();
	const vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder();

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
			initialWeights.Assign(p->first, p->second);
	}
	decoder->setWeights(initialWeights);

	//Main loop:
	// print initial weights
	cerr << "Rank " << rank << ", initial weights: " << initialWeights << endl;
	ScoreComponentCollection cumulativeWeights; // collect weights per epoch to produce an average
	ScoreComponentCollection cumulativeWeightsBinary;
	size_t numberOfUpdates = 0;
	size_t numberOfUpdatesThisEpoch = 0;

	time_t now;
	time(&now);
	cerr << "Rank " << rank << ", " << ctime(&now);

	float avgInputLength = 0;
	float sumOfInputs = 0;
	size_t numberOfInputs = 0;

	ScoreComponentCollection mixedWeights;
	ScoreComponentCollection mixedWeightsPrevious;
	ScoreComponentCollection mixedWeightsBeforePrevious;
	ScoreComponentCollection mixedAverageWeights;
	ScoreComponentCollection mixedAverageWeightsPrevious;
	ScoreComponentCollection mixedAverageWeightsBeforePrevious;

	// log feature counts and/or hope/fear translations with features
	string f1 = "decode_hope_epoch0";
	string f2 = "decode_fear_epoch0";
  ofstream hopePlusFeatures(f1.c_str());
  ofstream fearPlusFeatures(f2.c_str());
  if (!hopePlusFeatures || !fearPlusFeatures) {
  	ostringstream msg;
  	msg << "Unable to open file";
  	throw runtime_error(msg.str());
  }

	bool stop = false;
//	int sumStillViolatedConstraints;
	for (size_t epoch = 0; epoch < epochs && !stop; ++epoch) {
		// sum of violated constraints in an epoch
//		sumStillViolatedConstraints = 0;

		numberOfUpdatesThisEpoch = 0;
		// Sum up weights over one epoch, final average uses weights from last epoch
		if (!accumulateWeights) {
			cumulativeWeights.ZeroAll();
			cumulativeWeightsBinary.ZeroAll();
		}

		// number of weight dumps this epoch
		size_t weightMixingThisEpoch = 0;
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

			string nbestFileMegam, referenceFileMegam;
			vector<size_t>::const_iterator current_sid_start = sid;
			size_t examples_in_batch = 0;
			for (size_t batchPosition = 0; batchPosition < batchSize && sid
			    != shard.end(); ++batchPosition) {
				string input;
				if (trainWithMultipleFolds) 
					input = inputSentencesFolds[myFold][*sid];
				else
					input = inputSentences[*sid];
//				const vector<string>& refs = referenceSentences[*sid];
				cerr << "\nRank " << rank << ", epoch " << epoch << ", input sentence " << *sid << ": \"" << input << "\"" << " (batch pos " << batchPosition << ")" << endl;

				Moses::Sentence *sentence = new Sentence();
			    stringstream in(input + "\n");
			    sentence->Read(in,inputFactorOrder);
			    size_t current_input_length = (*sentence).GetSize();

				if (epoch == 0 && (scaleByAvgInputLength || scaleByAvgInverseLength)) {
					sumOfInputs += current_input_length;
					++numberOfInputs;
					avgInputLength = sumOfInputs/numberOfInputs;
					decoder->setAvgInputLength(avgInputLength);
					cerr << "Rank " << rank << ", epoch 0, average input length: " << avgInputLength << endl;
				}

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

				// preparation for MegaM
				if (megam) {
					ostringstream hope_nbest_filename, fear_nbest_filename, model_nbest_filename, ref_filename;
					ofstream dummy;
					cerr << "Generate nbest lists for megam.." << endl;
					hope_nbest_filename << "decode_hope_sent" << *sid << "." << hope_n << "best";
					fear_nbest_filename << "decode_fear_sent" << *sid << "." << fear_n << "best";
					//model_nbest_filename << "decode_model_sent" << *sid << "." << n << "best";
					cerr << "Writing file " << hope_nbest_filename.str() << endl;
					decoder->outputNBestList(input, *sid, hope_n, 1, bleuWeight_hope, distinctNbest,
							avgRefLength, hope_nbest_filename.str(), dummy);
					decoder->cleanup(chartDecoding);
					cerr << "Writing file " << fear_nbest_filename.str() << endl;
					decoder->outputNBestList(input, *sid, fear_n, -1, bleuWeight_fear, distinctNbest,
							avgRefLength, fear_nbest_filename.str(), dummy);
					decoder->cleanup(chartDecoding);
					//decoder->outputNBestList(input, *sid, n, 0, bleuWeight, distinctNbest,
						//model_nbest_filename.str());
					//decoder->cleanup(chartDecoding);

					// save reference
					ref_filename <<  "decode_ref_sent" << *sid;
					referenceFileMegam = ref_filename.str();
					ofstream ref_out(referenceFileMegam.c_str());
					if (!ref_out) {
						ostringstream msg;
						msg << "Unable to open " << referenceFileMegam;
						throw runtime_error(msg.str());
					}
					ref_out << referenceSentences[decoder->getShortestReferenceIndex(*sid)][*sid] << "\n";
					ref_out.close();

					// concatenate nbest files
					stringstream nbestStreamMegam, catCmd, sortCmd;
					nbestStreamMegam << "decode_hope-fear_sent" << *sid << "." << (hope_n+fear_n) << "best";
					string tmp = nbestStreamMegam.str();
					nbestFileMegam = tmp+".sorted";
					catCmd << "cat " << hope_nbest_filename.str() << " " << fear_nbest_filename.str() << " > " << tmp;
					sortCmd << "sort -k1,1n " << tmp << " > " << nbestFileMegam;
					system(catCmd.str().c_str());
					system(sortCmd.str().c_str());
				}

				if (hope_fear || hope_fear_rank || perceptron_update) {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << hope_n << "best hope translations" << endl;
					vector< vector<const Word*> > outputHope = decoder->getNBest(input, *sid, hope_n, 1.0, bleuWeight_hope,
							featureValuesHope[batchPosition], bleuScoresHope[batchPosition], modelScoresHope[batchPosition],
							1, distinctNbest, avgRefLength, rank, epoch);
					vector<const Word*> oracle = outputHope[0];
					decoder->cleanup(chartDecoding);
					ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
					avg_ref_length = ref_length;
					float hope_length_ratio = (float)oracle.size()/ref_length;
					int oracleSize = (int)oracle.size();
					cerr << ", l-ratio hope: " << hope_length_ratio << endl;
					cerr << "Rank " << rank << ", epoch " << epoch << ", current input length: " << current_input_length << endl;

					// count sparse features occurring in hope translation
					featureValuesHope[batchPosition][0].IncrementSparseHopeFeatures();

					if (epoch == 0 && printNbestWithFeatures) {
						decoder->outputNBestList(input, *sid, hope_n, 1, bleuWeight_hope, distinctNbest,
								avgRefLength, "", hopePlusFeatures);
						decoder->cleanup(chartDecoding);
					}

					
					float precision = bleuScoresHope[batchPosition][0];
					if (historyOf1best) {
						if (useSourceLengthHistory) precision /= decoder->getSourceLengthHistory();
						else precision /= decoder->getTargetLengthHistory();
					}
					else {
						if (scaleByAvgInputLength) precision /= decoder->getAverageInputLength();
						else if (scaleByAvgInverseLength) precision /= (100/decoder->getAverageInputLength());
						precision /= scaleByX;
					}
					if (scale_margin_precision || scale_update_precision) {
						if (historyOf1best || scaleByAvgInputLength || scaleByAvgInverseLength) {
							cerr << "Rank " << rank << ", epoch " << epoch << ", set hope precision: " << precision << endl;
							((MiraOptimiser*) optimiser)->setPrecision(precision);
						}
					}
					
//					exit(0);
					bool skip = false;

					// Length-related example selection
					float length_diff_hope = abs(1 - hope_length_ratio);
					if (max_length_dev_hope_ref != -1 && length_diff_hope > max_length_dev_hope_ref)
					  skip = true;

					vector<const Word*> bestModel;
					if (historyOf1best && !skip) {
						// MODEL (for updating the history only, using dummy vectors)
						cerr << "Rank " << rank << ", epoch " << epoch << ", 1best wrt model score (for history or length stabilisation)" << endl;
						vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, 1, 0.0, bleuWeight,
								dummyFeatureValues[batchPosition], dummyBleuScores[batchPosition], dummyModelScores[batchPosition],
								1, distinctNbest, avgRefLength, rank, epoch);
						bestModel = outputModel[0];
						decoder->cleanup(chartDecoding);
						cerr << endl;
						ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					}

					// FEAR
					float fear_length_ratio = 0;
					float bleuRatioHopeFear = 0;
					int fearSize = 0;
					if (!skip) {
						cerr << "Rank " << rank << ", epoch " << epoch << ", " << fear_n << "best fear translations" << endl;
						vector< vector<const Word*> > outputFear = decoder->getNBest(input, *sid, fear_n, -1.0, bleuWeight_fear,
								featureValuesFear[batchPosition], bleuScoresFear[batchPosition], modelScoresFear[batchPosition],
								1,	distinctNbest, avgRefLength, rank, epoch);
						vector<const Word*> fear = outputFear[0];
						decoder->cleanup(chartDecoding);
						ref_length = decoder->getClosestReferenceLength(*sid, fear.size());
						avg_ref_length += ref_length;
						avg_ref_length /= 2;
						fear_length_ratio = (float)fear.size()/ref_length;
						fearSize = (int)fear.size();
						cerr << ", l-ratio fear: " << fear_length_ratio << endl;
						for (size_t i = 0; i < fear.size(); ++i)
							delete fear[i];

						// count sparse features occurring in fear translation
						featureValuesFear[batchPosition][0].IncrementSparseFearFeatures();

						if (epoch == 0 && printNbestWithFeatures) {
							decoder->outputNBestList(input, *sid, fear_n, -1, bleuWeight_fear, distinctNbest,
									avgRefLength, "", fearPlusFeatures);
							decoder->cleanup(chartDecoding);
						}

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
					vector< vector<const Word*> > outputHope = decoder->getNBest(input, *sid, hope_n, 1.0, bleuWeight_hope,
							featureValuesHope[batchPosition], bleuScoresHope[batchPosition], modelScoresHope[batchPosition],
							1, distinctNbest, avgRefLength, rank, epoch);
					vector<const Word*> oracle = outputHope[0];
					decoder->cleanup(chartDecoding);
					cerr << endl;
					
					// count sparse features occurring in hope translation
					featureValuesHope[batchPosition][0].IncrementSparseHopeFeatures();

					vector<const Word*> bestModel;
					// MODEL (for updating the history only, using dummy vectors)
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << fear_n << "best wrt model score" << endl;
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, fear_n, 0.0, bleuWeight_fear,
							featureValuesFear[batchPosition], bleuScoresFear[batchPosition], modelScoresFear[batchPosition],
							1, distinctNbest, avgRefLength, rank, epoch);
					bestModel = outputModel[0];
					decoder->cleanup(chartDecoding);
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
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, rank_n, 0.0, bleuWeight,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, avgRefLength, rank, epoch);
					vector<const Word*> bestModel = outputModel[0];
					decoder->cleanup(chartDecoding);
					oneBests.push_back(bestModel);
					ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					float model_length_ratio = (float)bestModel.size()/ref_length;
					cerr << ", l-ratio model: " << model_length_ratio << endl;
					
					// count sparse features occurring in best model translation
					featureValues[batchPosition][0].IncrementSparseHopeFeatures();

					examples_in_batch++;
				}
				if (model_hope_fear) {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best hope translations" << endl;
					size_t oraclePos = featureValues[batchPosition].size();
					vector <vector<const Word*> > outputHope = decoder->getNBest(input, *sid, n, 1.0, bleuWeight_hope,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, avgRefLength, rank, epoch);
					vector<const Word*> oracle = outputHope[0];
					// needed for history
					inputLengths.push_back(current_input_length);
					ref_ids.push_back(*sid);
					decoder->cleanup(chartDecoding);
					oracles.push_back(oracle);
					ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
					float hope_length_ratio = (float)oracle.size()/ref_length;
					cerr << ", l-ratio hope: " << hope_length_ratio << endl;
					
					// count sparse features occurring in hope translation
					featureValues[batchPosition][0].IncrementSparseHopeFeatures();

					oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
					oracleBleuScores.push_back(bleuScores[batchPosition][oraclePos]);
					oracleModelScores.push_back(modelScores[batchPosition][oraclePos]);

					// MODEL
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best wrt model score" << endl;
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, n, 0.0, bleuWeight,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, avgRefLength, rank, epoch);
					vector<const Word*> bestModel = outputModel[0];
					decoder->cleanup(chartDecoding);
					oneBests.push_back(bestModel);
					ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					float model_length_ratio = (float)bestModel.size()/ref_length;
					cerr << ", l-ratio model: " << model_length_ratio << endl;

					// FEAR
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best fear translations" << endl;
//					size_t fearPos = featureValues[batchPosition].size();
					vector< vector<const Word*> > outputFear = decoder->getNBest(input, *sid, n, -1.0, bleuWeight_fear,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1, distinctNbest, avgRefLength, rank, epoch);
					vector<const Word*> fear = outputFear[0];
					decoder->cleanup(chartDecoding);
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

			if (megam) {
				// extract features and scores
				string scoreDataFile = "decode_hope-fear.scores.dat";
				string featureDataFile = "decode_hope-fear.features.dat";
				stringstream extractorCmd;
				extractorCmd << "/afs/inf.ed.ac.uk/user/s07/s0787953/mosesdecoder_github_saxnot/mert/extractor"
						" --scconfig case:true --scfile " << scoreDataFile << " --ffile " << featureDataFile <<
						" -r " << referenceFileMegam << " -n " << nbestFileMegam;
				system(extractorCmd.str().c_str());

				// pro --> select training examples
				stringstream proCmd;
				string proDataFile = "decode_hope-fear.pro.data";
				proCmd << "/afs/inf.ed.ac.uk/user/s07/s0787953/mosesdecoder_github_saxnot/mert/pro"
						" --ffile " << featureDataFile << " --scfile " << scoreDataFile << " -o " << proDataFile;
				system(proCmd.str().c_str());

				// megam
				stringstream megamCmd;
				string megamOut = "decode_hope-fear.megam-weights";
				string megamErr = "decode_hope-fear.megam-log";
				megamCmd << "/afs/inf.ed.ac.uk/user/s07/s0787953/mosesdecoder_github_saxnot/mert/megam_i686.opt"
						" -fvals -maxi 30 -nobias binary " << proDataFile << " 1> " << megamOut <<
						" 2> " << megamErr;
				system(megamCmd.str().c_str());

				// read feature order
				ifstream f_in(featureDataFile.c_str());
				if (!f_in)
					return false;
				string line;
				vector<string> coreFeats;
				if (getline(f_in, line)) {
					line = boost::algorithm::replace_all_copy(line, "  ", " ");
					boost::split(coreFeats, line, boost::is_any_of(" "));
				}
				else {
					cerr << "Error.." << endl;
					exit(1);
				}
				for (size_t i=0; i < coreFeats.size(); ++i)
					cerr << "from feature data file: " << coreFeats[i] << endl;

				// read megam optimized weights
				ifstream w_in(megamOut.c_str());
				if (!w_in)
					return false;

				while (getline(w_in, line)) {
					vector<string> keyValue;

					boost::split(keyValue, line, boost::is_any_of("\t "));

					// regular features
					if (keyValue[0].substr(0, 1).compare("F") == 0) {
						cerr << "core weight: " << keyValue[0] << " " << keyValue[1] << endl;
						//$WEIGHT[$1] = $2;
		        //$sum += abs($2);
		      }
		      // sparse features
		      else {
		        //$$sparse_weights{$1} = $2;
		      	cerr << "sparse weight: " << keyValue[0] << " " << keyValue[1] << endl;
		      }
				}

				// normalize weights
				//foreach (@WEIGHT) { $_ /= $sum; }
		    //foreach (keys %{$sparse_weights}) { $$sparse_weights{$_} /= $sum; }

			}
			else if (examples_in_batch == 0) {
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
				size_t update_status = 1;
				ScoreComponentCollection weightUpdate;
				if (perceptron_update) {
					vector<vector<float> > dummy1;
					update_status = optimiser->updateWeightsHopeFear(mosesWeights, weightUpdate,
							featureValuesHope, featureValuesFear, dummy1, dummy1, dummy1, dummy1, learning_rate, rank, epoch);
				}
				else if (hope_fear || hope_model) {
					if (bleuScoresHope[0][0] >= min_oracle_bleu)
						if (hope_n == 1 && fear_n ==1 && batchSize == 1)
							update_status = ((MiraOptimiser*) optimiser)->updateWeightsAnalytically(mosesWeights, weightUpdate,
									featureValuesHope[0][0], featureValuesFear[0][0], bleuScoresHope[0][0], bleuScoresFear[0][0],
									modelScoresHope[0][0], modelScoresFear[0][0], learning_rate, rank, epoch);
						else {
							if (batchSize > 1 && separateUpdates) {
								// separate updates for all input sentences
								ScoreComponentCollection tmpWeights(mosesWeights);
								for (size_t i = 0; i < batchSize; ++i) {
									// use only the specified batch position to compute the update
									int updatePosition = i;
									ScoreComponentCollection partialWeightUpdate;
									size_t partial_update_status = optimiser->updateWeightsHopeFear(tmpWeights, partialWeightUpdate,
										featureValuesHope, featureValuesFear, bleuScoresHope, bleuScoresFear,
										modelScoresHope, modelScoresFear, learning_rate, rank, epoch, updatePosition);
									if (partial_update_status == 0) {
										update_status = 0;
										weightUpdate.PlusEquals(partialWeightUpdate);
										tmpWeights.PlusEquals(partialWeightUpdate);
									}
								}
							}
							else
								update_status = optimiser->updateWeightsHopeFear(mosesWeights, weightUpdate,
									featureValuesHope, featureValuesFear, bleuScoresHope, bleuScoresFear,
									modelScoresHope, modelScoresFear, learning_rate, rank, epoch);
						}
				  else
				    update_status = 1;
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
					cerr << "Rank " << rank << ", epoch " << epoch << ", applying update.." << endl;
					mosesWeights.PlusEquals(weightUpdate);
					if (normaliseWeights)
						mosesWeights.L1Normalise();

					cumulativeWeights.PlusEquals(mosesWeights);
					if (sparseAverage) {
					  ScoreComponentCollection binary;
					  binary.SetToBinaryOf(mosesWeights);
					  cumulativeWeightsBinary.PlusEquals(binary);
					}

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
			} // END TRANSLATE AND UPDATE BATCH

			size_t mixing_base = mixingFrequency == 0 ? 0 : shard.size() / mixingFrequency;
			size_t dumping_base = weightDumpFrequency ==0 ? 0 : shard.size() / weightDumpFrequency;
			bool mix = evaluateModulo(shardPosition, mixing_base, actualBatchSize);

			// mix weights?
			if (mix) {
#ifdef MPI_ENABLE
				// collect all weights in mixedWeights and divide by number of processes
				mpi::reduce(world, mosesWeights, mixedWeights, SCCPlus(), 0);
				ScoreComponentCollection totalBinary;
				if (sparseAverage) {
					ScoreComponentCollection binary;
					binary.SetToBinaryOf(mosesWeights);
					mpi::reduce(world, binary, totalBinary, SCCPlus(), 0);
				}
				if (rank == 0) {
					// divide by number of processes
					if (sparseNoAverage)
					  mixedWeights.CoreDivideEquals(size); // average only core weights
					else if (sparseAverage)
					  mixedWeights.DivideEquals(totalBinary);
					else
					  mixedWeights.DivideEquals(size);

					// normalise weights after averaging
					if (normaliseWeights) {
						mixedWeights.L1Normalise();
					}
								
					++weightMixingThisEpoch;
					
					if (pruneZeroWeights) {
						size_t pruned = mixedWeights.PruneZeroWeightFeatures();
						cerr << "Rank " << rank << ", epoch " << epoch << ", " 
								<< pruned << " zero-weighted features pruned from mixedWeights." << endl;
						
						pruned = cumulativeWeights.PruneZeroWeightFeatures();
						cerr << "Rank " << rank << ", epoch " << epoch << ", " 
								<< pruned << " zero-weighted features pruned from cumulativeWeights." << endl;
					}
					
					if (featureCutoff != -1 && weightMixingThisEpoch == mixingFrequency) {
						size_t pruned = mixedWeights.PruneSparseFeatures(featureCutoff);
						cerr << "Rank " << rank << ", epoch " << epoch << ", " 
								<< pruned << " features pruned from mixedWeights." << endl;
						
						pruned = cumulativeWeights.PruneSparseFeatures(featureCutoff);
						cerr << "Rank " << rank << ", epoch " << epoch << ", " 
								<< pruned << " features pruned from cumulativeWeights." << endl;
					}
				}

				// broadcast average weights from process 0
				mpi::broadcast(world, mixedWeights, 0);
				decoder->setWeights(mixedWeights);
				mosesWeights = mixedWeights;
#endif
#ifndef MPI_ENABLE
				//				cerr << "\nRank " << rank << ", no mixing, weights: " << mosesWeights << endl;
				mixedWeights = mosesWeights;
#endif
			} // end mixing

			// Dump weights?
			if (dumpMixedWeights) {
			  if (mix && rank == 0 && !weightDumpStem.empty()) {
			    // dump mixed weights instead of average weights
			    ostringstream filename;
			    if (epoch < 10)
				  filename << weightDumpStem << "_0" << epoch;
			    else
				  filename << weightDumpStem << "_" << epoch;

			    if (weightDumpFrequency > 1)
				  filename << "_" << weightEpochDump;

			    cerr << "Dumping mixed weights during epoch " << epoch << " to " << filename.str() << endl << endl;
			    mixedWeights.Save(filename.str());
			    ++weightEpochDump;
			  }
			}
			else {
			if (evaluateModulo(shardPosition, dumping_base, actualBatchSize)) {
			  ScoreComponentCollection tmpAverageWeights(cumulativeWeights);
			  bool proceed = false;
			  if (accumulateWeights) {
			    if (numberOfUpdates > 0) {
			      tmpAverageWeights.DivideEquals(numberOfUpdates);
			      proceed = true;
			    }
			  } else {
			    if (numberOfUpdatesThisEpoch > 0) {
			    	if (sparseNoAverage) // average only core weights
			    		tmpAverageWeights.CoreDivideEquals(numberOfUpdatesThisEpoch);
			    	else if (sparseAverage)
			    		tmpAverageWeights.DivideEquals(cumulativeWeightsBinary);
			    	else
			    		tmpAverageWeights.DivideEquals(numberOfUpdatesThisEpoch);
			    	proceed = true;
			    }
			  }
			  
			  if (proceed) {
#ifdef MPI_ENABLE
			    // average across processes
				mpi::reduce(world, tmpAverageWeights, mixedAverageWeights, SCCPlus(), 0);
				ScoreComponentCollection totalBinary;
				if (sparseAverage) {
					ScoreComponentCollection binary;
					binary.SetToBinaryOf(mosesWeights);
					mpi::reduce(world, binary, totalBinary, SCCPlus(), 0);
				}
#endif
#ifndef MPI_ENABLE
			    mixedAverageWeights = tmpAverageWeights;
#endif
			    if (rank == 0 && !weightDumpStem.empty()) {
			      // divide by number of processes
			      if (sparseNoAverage)
			    	  mixedAverageWeights.CoreDivideEquals(size); // average only core weights
			      else if (sparseAverage)
			    	  mixedAverageWeights.DivideEquals(totalBinary);
			      else
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
			      
/*			      if (accumulateWeights) {
				  	cerr << "\nMixed average weights (cumulative) during epoch "	<< epoch << ": " << mixedAverageWeights << endl;
			      } else {
				  	cerr << "\nMixed average weights during epoch " << epoch << ": " << mixedAverageWeights << endl;
			      }*/
			      
			      cerr << "Dumping mixed average weights during epoch " << epoch << " to " << filename.str() << endl << endl;
			      mixedAverageWeights.Save(filename.str());
			      ++weightEpochDump;

			      if (weightEpochDump == weightDumpFrequency && printFeatureCounts) {
			      	// print out all features with counts
				stringstream s1, s2;
				s1 << "sparse_feature_hope_counts" << "_" << epoch;
				s2 << "sparse_feature_fear_counts" << "_" << epoch;
				ofstream sparseFeatureCountsHope(s1.str().c_str());
				ofstream sparseFeatureCountsFear(s2.str().c_str());

			      	cerr << "Printing out hope feature counts" << endl;
			      	mixedAverageWeights.PrintSparseHopeFeatureCounts(sparseFeatureCountsHope);
			      	cerr << "Printing out fear feature counts" << endl;
			      	mixedAverageWeights.PrintSparseFearFeatureCounts(sparseFeatureCountsFear);
			      	sparseFeatureCountsHope.close();
			      	sparseFeatureCountsFear.close();
			      }
			    }
			  }
			}// end dumping
			}

		} // end of shard loop, end of this epoch

		if (printNbestWithFeatures && rank == 0 && epoch == 0) {
      cerr << "Writing out hope/fear nbest list with features: " << f1 << ", " << f2 << endl;
			hopePlusFeatures.close();
			fearPlusFeatures.close();
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
					ScoreComponentCollection firstDiff, secondDiff;
					if (dumpMixedWeights) {
						firstDiff = mixedWeights;
						firstDiff.MinusEquals(mixedWeightsPrevious);
						secondDiff = mixedWeights;
						secondDiff.MinusEquals(mixedWeightsBeforePrevious);
					}
					else {
						firstDiff = mixedAverageWeights;
						firstDiff.MinusEquals(mixedAverageWeightsPrevious);
						secondDiff = mixedAverageWeights;
						secondDiff.MinusEquals(mixedAverageWeightsBeforePrevious);
					}
					VERBOSE(1, "Average weight changes since previous epoch: " << firstDiff << " (max: " << firstDiff.GetLInfNorm() << ")" << endl);
					VERBOSE(1, "Average weight changes since before previous epoch: " << secondDiff << " (max: " << secondDiff.GetLInfNorm() << ")" << endl << endl);

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

				mixedWeightsBeforePrevious = mixedWeightsPrevious;
				mixedWeightsPrevious = mixedWeights;
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
	while (getline(in, line))
		sentences.push_back(line);
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

void decodeHopeOrFear(size_t rank, size_t size, size_t decode, string filename, vector<string> &inputSentences, MosesDecoder* decoder, size_t n) {
	if (decode == 1)
		cerr << "Rank " << rank << ", decoding dev input set according to hope objective.. " << endl;
	else if (decode == 2)
		cerr << "Rank " << rank << ", decoding dev input set according to fear objective.. " << endl;
	else
		cerr << "Rank " << rank << ", decoding dev input set according to normal objective.. " << endl;

	// Create shards according to the number of processes used
	vector<size_t> order;
	for (size_t i = 0; i < inputSentences.size(); ++i)
		order.push_back(i);

	vector<size_t> shard;
	float shardSize = (float) (order.size()) / size;
	size_t shardStart = (size_t) (shardSize * rank);
	size_t shardEnd = (size_t) (shardSize * (rank + 1));
	if (rank == size - 1) {
		shardEnd = inputSentences.size();
		shardSize = shardEnd - shardStart;
	}
	VERBOSE(1, "Rank " << rank << ", shard start: " << shardStart << " Shard end: " << shardEnd << endl);
	VERBOSE(1, "Rank " << rank << ", shard size: " << shardSize << endl);
	shard.resize(shardSize);
	copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());

	// open files for writing
	stringstream fname;
	fname << filename << ".rank" << rank;
	filename = fname.str();
	ostringstream filename_nbest;
	filename_nbest << filename << "." << n << "best";
	ofstream out(filename.c_str());
	ofstream nbest_out((filename_nbest.str()).c_str());
	if (!out) {
		ostringstream msg;
		msg << "Unable to open " << fname.str();
		throw runtime_error(msg.str());
	}
	if (!nbest_out) {
		ostringstream msg;
		msg << "Unable to open " << filename_nbest;
		throw runtime_error(msg.str());
	}

	for (size_t i = 0; i < shard.size(); ++i) {
		size_t sid = shard[i];
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
		if (decode == 1) factor = 1.0;
		if (decode == 2) factor = -1.0;
		cerr << "Rank " << rank << ", translating sentence " << sid << endl;
		vector< vector<const Word*> > nbestOutput = decoder->getNBest(input, sid, n, factor, 1, dummyFeatureValues[0],
				dummyBleuScores[0], dummyModelScores[0], n, true, false, rank, 0);
		cerr << endl;
		decoder->cleanup(StaticData::Instance().GetSearchAlgorithm() == ChartDecoding);

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

#ifdef MPI_ENABLE
	MPI_Finalize();
#endif

	time_t now;
	time(&now);
	cerr << "Rank " << rank << ", " << ctime(&now);

	delete decoder;
	exit(0);
}
