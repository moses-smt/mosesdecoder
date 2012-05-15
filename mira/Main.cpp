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
#include "DummyScoreProducers.h"
#include "LexicalReordering.h"
#include "BleuScorer.h"

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
  string coreWeightFile, startWeightFile;
  size_t epochs;
  string learner;
  bool shuffle;
  size_t mixingFrequency;
  size_t weightDumpFrequency;
  string weightDumpStem;
  bool scale_margin, scale_margin_precision;
  bool scale_update, scale_update_precision;
  size_t n;
  size_t batchSize;
  bool distinctNbest;
  bool accumulateWeights;
  float historySmoothing;
  bool scaleByInputLength, scaleByAvgInputLength;
  bool scaleByInverseLength, scaleByAvgInverseLength;
  float scaleByX;
  float slack;
  bool averageWeights;
  bool weightConvergence;
  float learning_rate;
  float mira_learning_rate;
  float perceptron_learning_rate;
  string decoder_settings;
  float min_weight_change;
  bool normaliseWeights, normaliseMargin;
  bool print_feature_values;
  bool historyBleu   ;
  bool sentenceBleu;
  bool perceptron_update;
  bool hope_fear, hope_model;
  bool model_hope_fear, rank_only;
  int hope_n, fear_n, rank_n;
  size_t bleu_smoothing_scheme;
  float min_oracle_bleu;
  float minBleuRatio, maxBleuRatio;
  bool boost;
  bool decode_hope, decode_fear, decode_model;
  string decode_filename;
  bool batchEqualsShard;
  bool sparseAverage, dumpMixedWeights, sparseNoAverage;
  int featureCutoff;
  bool pruneZeroWeights;
  bool megam;
  bool printFeatureCounts, printNbestWithFeatures;
  bool avgRefLength;
  bool print_weights, print_core_weights, clear_static, debug_model, scale_lm, scale_wp;
  float scale_lm_factor, scale_wp_factor;
  bool sample;
  string moses_src;
  bool external_score = false;
  float sigmoidParam;
  float bleuWeight, bleuWeight_hope, bleuWeight_fear;
  bool bleu_weight_lm, bleu_weight_lm_adjust;
  float bleu_weight_lm_factor;
  bool scale_all;
  float scale_all_factor;
  bool l1_regularize, l2_regularize;
  float l1_lambda, l2_lambda;
  bool most_violated, all_violated, max_bleu_diff, one_against_all;
  bool feature_confidence, signed_counts;
  float decay;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("bleu-weight", po::value<float>(&bleuWeight)->default_value(1.0), "Bleu weight used in decoder objective")
    ("bw-hope", po::value<float>(&bleuWeight_hope)->default_value(-1.0), "Bleu weight used in decoder objective for hope")
    ("bw-fear", po::value<float>(&bleuWeight_fear)->default_value(-1.0), "Bleu weight used in decoder objective for fear")
    
    ("tie-bw-to-lm", po::value<bool>(&bleu_weight_lm)->default_value(false), "Make bleu weight depend on lm weight")   
    ("adjust-bw", po::value<bool>(&bleu_weight_lm_adjust)->default_value(false), "Adjust bleu weight when lm weight changes")       
    ("bw-lm-factor", po::value<float>(&bleu_weight_lm_factor)->default_value(2.0), "Make bleu weight depend on lm weight by this factor")
    
    ("scale-all", po::value<bool>(&scale_all)->default_value(false), "Scale all core features")
    ("scaling-factor", po::value<float>(&scale_all_factor)->default_value(2), "Scaling factor for all core features")
    
    ("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
    ("average-weights", po::value<bool>(&averageWeights)->default_value(false), "Set decoder weights to average weights after each update")
    ("avg-ref-length", po::value<bool>(&avgRefLength)->default_value(false), "Use average reference length instead of shortest for BLEU score feature")
    ("batch-equals-shard", po::value<bool>(&batchEqualsShard)->default_value(false), "Batch size is equal to shard size (purely batch)")
    ("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
    ("bleu-smoothing-scheme", po::value<size_t>(&bleu_smoothing_scheme)->default_value(1), "Set a smoothing scheme for sentence-Bleu: +1 (1), +0.1 (2), papineni (3) (default:1)")
    ("boost", po::value<bool>(&boost)->default_value(false), "Apply boosting factor to updates on misranked candidates")
    ("clear-static", po::value<bool>(&clear_static)->default_value(false), "Clear static data before every translation")
    ("config,f", po::value<string>(&mosesConfigFile), "Moses ini-file")
    ("configs-folds", po::value<vector<string> >(&mosesConfigFilesFolds), "Moses ini-files, one for each fold")
    ("core-weights", po::value<string>(&coreWeightFile)->default_value(""), "Weight file containing the core weights (already tuned, have to be non-zero)")
    ("decay", po::value<float>(&decay)->default_value(0.01), "Decay factor for updating feature learning rates")
    ("debug-model", po::value<bool>(&debug_model)->default_value(false), "Get best model translation for debugging purposes")
    ("decode-hope", po::value<bool>(&decode_hope)->default_value(false), "Decode dev input set according to hope objective")
    ("decode-fear", po::value<bool>(&decode_fear)->default_value(false), "Decode dev input set according to fear objective")
    ("decode-model", po::value<bool>(&decode_model)->default_value(false), "Decode dev input set according to normal objective")
    ("decode-filename", po::value<string>(&decode_filename), "Filename for Bleu objective translations")
    ("decoder-settings", po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
    ("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(true), "Use n-best list with distinct translations in inference step")
    ("dump-mixed-weights", po::value<bool>(&dumpMixedWeights)->default_value(false), "Dump mixed weights instead of averaged weights")
    ("epochs,e", po::value<size_t>(&epochs)->default_value(10), "Number of epochs")
    ("feature-confidence", po::value<bool>(&feature_confidence)->default_value(false), "Use feature weight confidence in weight updates")
    ("feature-cutoff", po::value<int>(&featureCutoff)->default_value(-1), "Feature cutoff as additional regularization for sparse features")
    ("fear-n", po::value<int>(&fear_n)->default_value(-1), "Number of fear translations used")
    ("help", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
    ("history-bleu", po::value<bool>(&historyBleu)->default_value(false), "Use 1best translations to update the history")
    ("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.9), "Adjust the factor for history smoothing")
    ("hope-fear", po::value<bool>(&hope_fear)->default_value(true), "Use only hope and fear translations for optimisation (not model)")
    ("hope-model", po::value<bool>(&hope_model)->default_value(false), "Use only hope and model translations for optimisation (use --fear-n to set number of model translations)")
    ("hope-n", po::value<int>(&hope_n)->default_value(-1), "Number of hope translations used")
    ("input-file,i", po::value<string>(&inputFile), "Input file containing tokenised source")
    ("input-files-folds", po::value<vector<string> >(&inputFilesFolds), "Input files containing tokenised source, one for each fold")
    ("learner,l", po::value<string>(&learner)->default_value("mira"), "Learning algorithm")
    ("l1-lambda", po::value<float>(&l1_lambda)->default_value(0.001), "Lambda for l1-regularization (w_i +/- lambda)")
    ("l2-lambda", po::value<float>(&l2_lambda)->default_value(0.1), "Lambda for l2-regularization (w_i * (1 - lambda))")
    ("l1-reg", po::value<bool>(&l1_regularize)->default_value(false), "L1-regularization")
    ("l2-reg", po::value<bool>(&l2_regularize)->default_value(false), "L2-regularization")
    ("min-bleu-ratio", po::value<float>(&minBleuRatio)->default_value(-1), "Set a minimum BLEU ratio between hope and fear")
    ("max-bleu-ratio", po::value<float>(&maxBleuRatio)->default_value(-1), "Set a maximum BLEU ratio between hope and fear")
    ("max-bleu-diff", po::value<bool>(&max_bleu_diff)->default_value(true), "For 'sampling': select hope/fear with maximum Bleu difference")
    ("megam", po::value<bool>(&megam)->default_value(false), "Use megam for optimization step")
    ("min-oracle-bleu", po::value<float>(&min_oracle_bleu)->default_value(0), "Set a minimum oracle BLEU score")
    ("min-weight-change", po::value<float>(&min_weight_change)->default_value(0.01), "Set minimum weight change for stopping criterion")
    ("mira-learning-rate", po::value<float>(&mira_learning_rate)->default_value(1), "Learning rate for MIRA (fixed or flexible)")
    ("mixing-frequency", po::value<size_t>(&mixingFrequency)->default_value(1), "How often per epoch to mix weights, when using mpi")
    ("model-hope-fear", po::value<bool>(&model_hope_fear)->default_value(false), "Use model, hope and fear translations for optimisation")
    ("moses-src", po::value<string>(&moses_src)->default_value(""), "Moses source directory")
    ("most-violated", po::value<bool>(&most_violated)->default_value(false), "Pick pair of hypo and hope that violates constraint the most")
    ("all-violated", po::value<bool>(&all_violated)->default_value(false), "Pair all hypos with hope translation that violate constraint")
    ("one-against-all", po::value<bool>(&one_against_all)->default_value(false), "Pick best Bleu as hope and all others are fear")
    ("nbest,n", po::value<size_t>(&n)->default_value(1), "Number of translations in n-best list")
    ("normalise-weights", po::value<bool>(&normaliseWeights)->default_value(false), "Whether to normalise the updated weights before passing them to the decoder")
    ("normalise-margin", po::value<bool>(&normaliseMargin)->default_value(false), "Normalise the margin: squash between 0 and 1")
    ("perceptron-learning-rate", po::value<float>(&perceptron_learning_rate)->default_value(0.01), "Perceptron learning rate")
    ("print-feature-values", po::value<bool>(&print_feature_values)->default_value(false), "Print out feature values")
    ("print-feature-counts", po::value<bool>(&printFeatureCounts)->default_value(false), "Print out feature values, print feature list with hope counts after 1st epoch")
    ("print-nbest-with-features", po::value<bool>(&printNbestWithFeatures)->default_value(false), "Print out feature values, print feature list with hope counts after 1st epoch")
    ("print-weights", po::value<bool>(&print_weights)->default_value(false), "Print out current weights")
    ("print-core-weights", po::value<bool>(&print_core_weights)->default_value(true), "Print out current core weights")
    ("prune-zero-weights", po::value<bool>(&pruneZeroWeights)->default_value(false), "Prune zero-valued sparse feature weights")	    
    ("rank-n", po::value<int>(&rank_n)->default_value(-1), "Number of translations used for ranking")
    ("rank-only", po::value<bool>(&rank_only)->default_value(false), "Use only model translations for optimisation")
    ("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training")
    ("reference-files-folds", po::value<vector<string> >(&referenceFilesFolds), "Reference translation files for training, one for each fold")	       
    ("sample", po::value<bool>(&sample)->default_value(false), "Sample a translation pair from hope/(model)/fear translations") 
    ("scale-by-inverse-length", po::value<bool>(&scaleByInverseLength)->default_value(false), "Scale BLEU by (history of) inverse input length")
    ("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale BLEU by (history of) input length")
    ("scale-by-avg-input-length", po::value<bool>(&scaleByAvgInputLength)->default_value(false), "Scale BLEU by average input length")
    ("scale-by-avg-inverse-length", po::value<bool>(&scaleByAvgInverseLength)->default_value(false), "Scale BLEU by average inverse input length")
    ("scale-by-x", po::value<float>(&scaleByX)->default_value(1), "Scale the BLEU score by value x")
    ("scale-lm", po::value<bool>(&scale_lm)->default_value(false), "Scale the language model feature") 
    ("scale-factor-lm", po::value<float>(&scale_lm_factor)->default_value(2), "Scale the language model feature by this factor")
    ("scale-wp", po::value<bool>(&scale_wp)->default_value(false), "Scale the word penalty feature") 
    ("scale-factor-wp", po::value<float>(&scale_wp_factor)->default_value(2), "Scale the word penalty feature by this factor")
    ("scale-margin", po::value<bool>(&scale_margin)->default_value(0), "Scale the margin by the Bleu score of the oracle translation")
    ("scale-margin-precision", po::value<bool>(&scale_margin_precision)->default_value(0), "Scale margin by precision of oracle")
    ("scale-update", po::value<bool>(&scale_update)->default_value(0), "Scale update by Bleu score of oracle") 
    ("scale-update-precision", po::value<bool>(&scale_update_precision)->default_value(0), "Scale update by precision of oracle")	
    ("sentence-level-bleu", po::value<bool>(&sentenceBleu)->default_value(true), "Use a sentences level Bleu scoring function")
    ("shuffle", po::value<bool>(&shuffle)->default_value(false), "Shuffle input sentences before processing")
    ("signed-counts", po::value<bool>(&signed_counts)->default_value(true), "use signed counts for feature learning rates")
    ("sigmoid-param", po::value<float>(&sigmoidParam)->default_value(1), "y=sigmoidParam is the axis that this sigmoid approaches")
    ("slack", po::value<float>(&slack)->default_value(0.01), "Use slack in optimiser")
    ("sparse-average", po::value<bool>(&sparseAverage)->default_value(false), "Average weights by the number of processes")
    ("sparse-no-average", po::value<bool>(&sparseNoAverage)->default_value(false), "Don't average sparse weights, just sum")
    ("start-weights", po::value<string>(&startWeightFile)->default_value(""), "Weight file containing start weights")
    ("stop-weights", po::value<bool>(&weightConvergence)->default_value(true), "Stop when weights converge")
    ("verbosity,v", po::value<int>(&verbosity)->default_value(0), "Verbosity level")
    ("weight-dump-frequency", po::value<size_t>(&weightDumpFrequency)->default_value(1), "How often per epoch to dump weights (mpi)")
    ("weight-dump-stem", po::value<string>(&weightDumpStem)->default_value("weights"), "Stem of filename to use for dumping weights");
  
  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv). options(cmdline_options).run(), vm);
  po::notify(vm);
  
  if (help) {
    std::cout << "Usage: " + string(argv[0])
      + " -f mosesini-file -i input-file -r reference-file(s) [options]" << std::endl;
    std::cout << desc << std::endl;
    return 0;
  }
  
  const StaticData &staticData = StaticData::Instance();

  cerr << "scale-all: " << scale_all << endl;
  cerr << "scale-all-factor: " << scale_all_factor << endl;
  cerr << "bleu weight: " << bleuWeight << endl;
  cerr << "bleu weight hope: " << bleuWeight_hope << endl;
  cerr << "bleu weight fear: " << bleuWeight_fear << endl;;
  cerr << "bleu weight depends on lm: " << bleu_weight_lm << endl;
  cerr << "by this factor: " << bleu_weight_lm_factor << endl;
  cerr << "adjust dynamically: " << bleu_weight_lm_adjust << endl;
  cerr << "l1-reg: " << l1_regularize << endl;
  cerr << "l1-lambda: " << l1_lambda << endl;
  cerr << "l2-reg: " << l2_regularize << endl;
  cerr << "l2-lambda: " << l2_lambda << endl;

  bool trainWithMultipleFolds = false; 
  if (mosesConfigFilesFolds.size() > 0 || inputFilesFolds.size() > 0 || referenceFilesFolds.size() > 0) {
  	if (rank == 0)
  		cerr << "Training with " << mosesConfigFilesFolds.size() << " folds" << endl;
  	trainWithMultipleFolds = true;
  }

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

	if (historyBleu) {
	  sentenceBleu = false;
	  cerr << "Using history Bleu. " << endl;
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
	decoder->setBleuParameters(sentenceBleu, scaleByInputLength, scaleByAvgInputLength,
			scaleByInverseLength, scaleByAvgInverseLength,
			scaleByX, historySmoothing, bleu_smoothing_scheme);
	SearchAlgorithm searchAlgorithm = staticData.GetSearchAlgorithm();
	bool chartDecoding = (searchAlgorithm == ChartDecoding);

	// Optionally shuffle the sentences
	vector<size_t> order;
	if (trainWithMultipleFolds) {  	
	  for (size_t i = 0; i < inputSentencesFolds[myFold].size(); ++i) {
	    order.push_back(i);
	  }
	}
	else {
	  if (rank == 0) {
	    for (size_t i = 0; i < inputSentences.size(); ++i) {
	      order.push_back(i);
	    }	   
	  }
	}

	// initialise optimizer
	Optimiser* optimiser = NULL;
	if (learner == "mira") {
		if (rank == 0) {
			cerr << "Optimising using Mira" << endl;
			cerr << "slack: " << slack << ", learning rate: " << mira_learning_rate << endl;
			if (normaliseMargin) 
			  cerr << "sigmoid parameter: " << sigmoidParam << endl;
		}
		optimiser = new MiraOptimiser(slack, scale_margin, scale_margin_precision,
					      scale_update, scale_update_precision, boost, normaliseMargin, sigmoidParam);
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

	if (sample)
		model_hope_fear = true;
	if (model_hope_fear || hope_model || rank_only || megam)
		hope_fear = false; // is true by default
	if (learner == "mira" && !(hope_fear || hope_model || model_hope_fear || rank_only || megam)) {
		cerr << "Error: Need to select one of parameters --hope-fear/--model-hope-fear for mira update." << endl;
		return 1;
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
	//const vector<FactorType> &inputFactorOrder = staticData.GetInputFactorOrder();

	ProducerWeightMap coreWeightMap, startWeightMap;
	ScoreComponentCollection initialWeights = decoder->getWeights();
	// read start weight file                                                                                
	if (!startWeightFile.empty()) {
	  if (!loadCoreWeights(startWeightFile, startWeightMap, featureFunctions)) {
	    cerr << "Error: Failed to load start weights from " << startWeightFile << endl;
	    return 1;
	  }
	  else
	    cerr << "Loaded start weights from " << startWeightFile << "." << endl;
	       
	  // set start weights                                                                                                          
	  if (startWeightMap.size() > 0) {
	    ProducerWeightMap::iterator p;
	    for(p = startWeightMap.begin(); p!=startWeightMap.end(); ++p)
              initialWeights.Assign(p->first, p->second);
	  }
	}

	// read core weight file
	if (!coreWeightFile.empty()) {
	  if (!loadCoreWeights(coreWeightFile, coreWeightMap, featureFunctions)) {
	    cerr << "Error: Failed to load core weights from " << coreWeightFile << endl;
	    return 1;
	  }
	  else
	    cerr << "Loaded core weights from " << coreWeightFile << "." << endl;
		
	  // set core weights
	  if (coreWeightMap.size() > 0) {
	    ProducerWeightMap::iterator p;
	    for(p = coreWeightMap.begin(); p!=coreWeightMap.end(); ++p)
	      initialWeights.Assign(p->first, p->second);
	  }
	}
        cerr << "Rank " << rank << ", initial weights: " << initialWeights << endl;
	
	if (normaliseWeights) {
	  initialWeights.L1Normalise();
	  cerr << "Rank " << rank << ", normalised initial weights: " << initialWeights << endl;
	}
	decoder->setWeights(initialWeights);

	if (scale_all) {
	  cerr << "Scale all core features by factor " << scale_all_factor << endl;
	  scale_lm = true;
	  scale_wp = true;
	  scale_lm_factor = scale_all_factor;
	  scale_wp_factor = scale_all_factor;
	}

	// set bleu weight to twice the size of the language model weight(s)
	const LMList& lmList = staticData.GetLMList();
	if (bleu_weight_lm) {
	  float lmSum = 0;
	  for (LMList::const_iterator i = lmList.begin(); i != lmList.end(); ++i) 
	    lmSum += abs(initialWeights.GetScoreForProducer(*i));
	  bleuWeight = lmSum * bleu_weight_lm_factor;
	  cerr << "Set bleu weight to lm weight * " << bleu_weight_lm_factor << endl;
	}

	if (bleuWeight_hope == -1) {
	  bleuWeight_hope = bleuWeight;
	}
	if (bleuWeight_fear == -1) {
	  bleuWeight_fear = bleuWeight;
	}
	cerr << "Bleu weight: " << bleuWeight << endl;

	if (decode_hope || decode_fear || decode_model) {
	  size_t decode = 1;
	  if (decode_fear) decode = 2;
	  if (decode_model) decode = 3;
	  decodeHopeOrFear(rank, size, decode, decode_filename, inputSentences, decoder, n, bleuWeight);
	}

	//Main loop:	
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
	float epsilon = 0.0001;

	// variables for feature confidence
	ScoreComponentCollection confidenceCounts, mixedConfidenceCounts, featureLearningRates;
	featureLearningRates.UpdateLearningRates(decay, confidenceCounts); //initialise core learning rates

	for (size_t epoch = 0; epoch < epochs && !stop; ++epoch) {
	  if (shuffle) {
	    if (trainWithMultipleFolds || rank == 0) {
	      cerr << "Rank " << rank << ", epoch " << epoch << ", shuffling input sentences.." << endl;
	      RandomIndex rindex;
	      random_shuffle(order.begin(), order.end(), rindex);
	    }

#ifdef MPI_ENABLE
	    if (!trainWithMultipleFolds)
	      mpi::broadcast(world, order, 0);
#endif

	    // redo shards 
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
	  }
	  
	  // sum of violated constraints in an epoch
	  // sumStillViolatedConstraints = 0;

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
			vector<vector<ScoreComponentCollection> > featureValuesHope, featureValuesHopeSample;
			vector<vector<ScoreComponentCollection> > featureValuesFear, featureValuesFearSample;
			vector<vector<float> > bleuScoresHope, bleuScoresHopeSample;
			vector<vector<float> > bleuScoresFear, bleuScoresFearSample;
			vector<vector<float> > modelScoresHope, modelScoresHopeSample;
			vector<vector<float> > modelScoresFear, modelScoresFearSample;
			
			// get moses weights
			ScoreComponentCollection mosesWeights = decoder->getWeights();
			VERBOSE(1, "\nRank " << rank << ", epoch " << epoch << ", weights: " << mosesWeights << endl);

			// BATCHING: produce nbest lists for all input sentences in batch
			vector<float> oracleBleuScores;
			vector<float> oracleModelScores;
			vector<vector<const Word*> > oneBests;
			vector<ScoreComponentCollection> oracleFeatureValues;
			vector<size_t> inputLengths;
			vector<size_t> ref_ids;
			size_t actualBatchSize = 0;

			string nbestFileMegam, referenceFileMegam;
			vector<size_t>::const_iterator current_sid_start = sid;
			size_t examples_in_batch = 0;
			bool skip_sample = false;
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
			    const vector<FactorType> inputFactorOrder = staticData.GetInputFactorOrder();
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
				if (model_hope_fear || rank_only) {
					featureValues.push_back(newFeatureValues);
					bleuScores.push_back(newScores);
					modelScores.push_back(newScores);
				}
				if (hope_fear || hope_model || perceptron_update) {
					featureValuesHope.push_back(newFeatureValues);
					featureValuesFear.push_back(newFeatureValues);
					bleuScoresHope.push_back(newScores);
					bleuScoresFear.push_back(newScores);
					modelScoresHope.push_back(newScores);
					modelScoresFear.push_back(newScores);
					if (historyBleu || debug_model) {
						featureValues.push_back(newFeatureValues);
						bleuScores.push_back(newScores);
						modelScores.push_back(newScores);
					}
				}
				if (sample) {
					featureValuesHopeSample.push_back(newFeatureValues);
					featureValuesFearSample.push_back(newFeatureValues);
					bleuScoresHopeSample.push_back(newScores);
					bleuScoresFearSample.push_back(newScores);
					modelScoresHopeSample.push_back(newScores);
					modelScoresFearSample.push_back(newScores);
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
					model_nbest_filename << "decode_model_sent" << *sid << "." << n << "best";
					
					cerr << "Writing file " << hope_nbest_filename.str() << endl;
					decoder->outputNBestList(input, *sid, hope_n, 1, bleuWeight_hope, distinctNbest,
							avgRefLength, hope_nbest_filename.str(), dummy);
					decoder->cleanup(chartDecoding);
					
					cerr << "Writing file " << fear_nbest_filename.str() << endl;
					decoder->outputNBestList(input, *sid, fear_n, -1, bleuWeight_fear, distinctNbest,
							avgRefLength, fear_nbest_filename.str(), dummy);
					decoder->cleanup(chartDecoding);
					
					cerr << "Writing file " << model_nbest_filename.str() << endl;
					decoder->outputNBestList(input, *sid, n, 0, bleuWeight, distinctNbest,
							avgRefLength, model_nbest_filename.str(), dummy);
					decoder->cleanup(chartDecoding);

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

					// concatenate nbest files (use hope and fear lists to extract samples from)
					stringstream nbestStreamMegam, catCmd, sortCmd;
					nbestStreamMegam << "decode_hope-model-fear_sent" << *sid << "." << (hope_n+n+fear_n) << "best";
					string tmp = nbestStreamMegam.str();
					//nbestFileMegam = tmp+".sorted";
					nbestFileMegam = tmp;
					catCmd << "cat " << hope_nbest_filename.str() << " " << model_nbest_filename.str() 
							<< " " << fear_nbest_filename.str() << " > " << tmp;
					//sortCmd << "sort -k 1 " << tmp << " > " << nbestFileMegam;
					system(catCmd.str().c_str());
					//system(sortCmd.str().c_str());
				}

				if (print_weights)
				  cerr << "Rank " << rank << ", epoch " << epoch << ", current weights: " << mosesWeights << endl;
				if (print_core_weights) {
				  cerr << "Rank " << rank << ", epoch " << epoch << ", current weights: ";
				  mosesWeights.PrintCoreFeatures(); 
				  cerr << endl;
				}
				 								  
				// check LM weight
				const LMList& lmList_new = staticData.GetLMList();
				for (LMList::const_iterator i = lmList_new.begin(); i != lmList_new.end(); ++i) {
				  float lmWeight = mosesWeights.GetScoreForProducer(*i);
				  cerr << "Rank " << rank << ", epoch " << epoch << ", lm weight: " << lmWeight << endl;
				  if (lmWeight <= 0) {
				    cerr << "Rank " << rank << ", epoch " << epoch << ", ERROR: language model weight should never be <= 0." << endl;
				    mosesWeights.Assign(*i, 0.1);
				    cerr << "Rank " << rank << ", epoch " << epoch << ", assign lm weights of 0.1" << endl;
				  }
				}
				
				// select inference scheme
				if (hope_fear || perceptron_update) {				  
				  if (clear_static) {
				    delete decoder;
				    StaticData::ClearDataStatic();
				    decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
				    decoder->setBleuParameters(sentenceBleu, scaleByInputLength, scaleByAvgInputLength, scaleByInverseLength, scaleByAvgInverseLength, scaleByX, historySmoothing, bleu_smoothing_scheme);
				    decoder->setWeights(mosesWeights);
				  }    
					
					// HOPE		 				  
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << hope_n << "best hope translations" << endl;
					vector< vector<const Word*> > outputHope = decoder->getNBest(input, *sid, hope_n, 1.0, bleuWeight_hope,
							featureValuesHope[batchPosition], bleuScoresHope[batchPosition], modelScoresHope[batchPosition],
							1, distinctNbest, avgRefLength, rank, epoch, "");
					vector<const Word*> oracle = outputHope[0];
					decoder->cleanup(chartDecoding);
					ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
					avg_ref_length = ref_length;
					float hope_length_ratio = (float)oracle.size()/ref_length;
					int oracleSize = (int)oracle.size();
					cerr << endl;

					//exit(0);

					// count sparse features occurring in hope translation
					featureValuesHope[batchPosition][0].IncrementSparseHopeFeatures();

					if (epoch == 0 && printNbestWithFeatures) {
						decoder->outputNBestList(input, *sid, hope_n, 1, bleuWeight_hope, distinctNbest,
								avgRefLength, "", hopePlusFeatures);
						decoder->cleanup(chartDecoding);
					}

					
					float precision = bleuScoresHope[batchPosition][0];
					if (historyBleu) {
					  precision /= decoder->getTargetLengthHistory();
					}
					else {
						if (scaleByAvgInputLength) precision /= decoder->getAverageInputLength();
						else if (scaleByAvgInverseLength) precision /= (100/decoder->getAverageInputLength());
						precision /= scaleByX;
					}
					if (scale_margin_precision || scale_update_precision) {
						if (historyBleu || scaleByAvgInputLength || scaleByAvgInverseLength) {
							cerr << "Rank " << rank << ", epoch " << epoch << ", set hope precision: " << precision << endl;
							((MiraOptimiser*) optimiser)->setPrecision(precision);
						}
					}
				
					vector<const Word*> bestModel;
					if (debug_model || historyBleu) {
						// MODEL (for updating the history only, using dummy vectors)
					  if (clear_static) {
                                            delete decoder;
					    StaticData::ClearDataStatic();
                                            decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
                                            decoder->setBleuParameters(sentenceBleu, scaleByInputLength, scaleByAvgInputLength, scaleByInverseLength, scaleByAvgInverseLength, scaleByX, historySmoothing, bleu_smoothing_scheme);
                                            decoder->setWeights(mosesWeights);
                                          }

						cerr << "Rank " << rank << ", epoch " << epoch << ", 1best wrt model score (debug or history)" << endl;
						vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, n, 0.0, bleuWeight,
								featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
								1, distinctNbest, avgRefLength, rank, epoch, "");
						bestModel = outputModel[0];
						decoder->cleanup(chartDecoding);
						cerr << endl;
						ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					}

					// FEAR
					float fear_length_ratio = 0;
					float bleuRatioHopeFear = 0;
					int fearSize = 0;
					if (clear_static) {
						delete decoder;
					    StaticData::ClearDataStatic();
					    decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
					    decoder->setBleuParameters(sentenceBleu, scaleByInputLength, scaleByAvgInputLength, scaleByInverseLength, scaleByAvgInverseLength, scaleByX, historySmoothing, bleu_smoothing_scheme);
					    decoder->setWeights(mosesWeights);
					}

					cerr << "Rank " << rank << ", epoch " << epoch << ", " << fear_n << "best fear translations" << endl;
					vector< vector<const Word*> > outputFear = decoder->getNBest(input, *sid, fear_n, -1.0, bleuWeight_fear,
							featureValuesFear[batchPosition], bleuScoresFear[batchPosition], modelScoresFear[batchPosition],
							1,	distinctNbest, avgRefLength, rank, epoch, "");
					vector<const Word*> fear = outputFear[0];
					decoder->cleanup(chartDecoding);
					ref_length = decoder->getClosestReferenceLength(*sid, fear.size());
					avg_ref_length += ref_length;
					avg_ref_length /= 2;
					fear_length_ratio = (float)fear.size()/ref_length;
					fearSize = (int)fear.size();
					cerr << endl;
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
					bool skip = false;
					bleuRatioHopeFear = bleuScoresHope[batchPosition][0] / bleuScoresFear[batchPosition][0];
					if (minBleuRatio != -1 && bleuRatioHopeFear < minBleuRatio)
						skip = true;
					if(maxBleuRatio != -1 && bleuRatioHopeFear > maxBleuRatio)
						skip = true;					

					// sanity check
					if (historyBleu) {
					  if (bleuScores[batchPosition][0] > bleuScoresHope[batchPosition][0] &&
					      modelScores[batchPosition][0] > modelScoresHope[batchPosition][0]) {
					    if (abs(bleuScores[batchPosition][0] - bleuScoresHope[batchPosition][0]) > epsilon &&
						abs(modelScores[batchPosition][0] - modelScoresHope[batchPosition][0]) > epsilon) {
					      cerr << "Rank " << rank << ", epoch " << epoch << ", ERROR: MODEL translation better than HOPE translation." << endl;
					      skip = true;
					    }
					  }
					  if (bleuScoresFear[batchPosition][0] > bleuScores[batchPosition][0] &&
					      modelScoresFear[batchPosition][0] > modelScores[batchPosition][0]) {
					    if (abs(bleuScoresFear[batchPosition][0] - bleuScores[batchPosition][0]) > epsilon && 
						abs(modelScoresFear[batchPosition][0] - modelScores[batchPosition][0]) > epsilon) {
					      cerr << "Rank " << rank << ", epoch " << epoch << ", ERROR: FEAR translation better than MODEL translation." << endl;
					      skip = true;
					    }					  
					  }
					}
					if (bleuScoresFear[batchPosition][0] > bleuScoresHope[batchPosition][0]) {
					  if (abs(bleuScoresFear[batchPosition][0] - bleuScoresHope[batchPosition][0]) > epsilon) {
					    // check if it's an error or a warning
					    skip = true;
					    if (modelScoresFear[batchPosition][0] > modelScoresHope[batchPosition][0] && abs(modelScoresFear[batchPosition][0] - modelScoresHope[batchPosition][0]) > epsilon) {
					      cerr << "Rank " << rank << ", epoch " << epoch << ", ERROR: FEAR translation better than HOPE translation. (abs-diff: " << abs(bleuScoresFear[batchPosition][0] - bleuScoresHope[batchPosition][0]) << ")" <<endl;			   
					    }
					    else {
					      cerr << "Rank " << rank << ", epoch " << epoch << ", WARNING: FEAR translation has better Bleu than HOPE translation. (abs-diff: " << abs(bleuScoresFear[batchPosition][0] - bleuScoresHope[batchPosition][0]) << ")" <<endl;
					    }
					  }
					}
									
					if (skip) {
					  cerr << "Rank " << rank << ", epoch " << epoch << ", skip example (" << hope_length_ratio << ", " << bleuRatioHopeFear << ").. " << endl;
					  featureValuesHope[batchPosition].clear();
					  featureValuesFear[batchPosition].clear();
					  bleuScoresHope[batchPosition].clear();
					  bleuScoresFear[batchPosition].clear();
					  if (historyBleu || debug_model) {
					    featureValues[batchPosition].clear();
					    bleuScores[batchPosition].clear();
					  }
					}
					else {
					  examples_in_batch++;

					  // needed for history
					  if (historyBleu)  {
					    inputLengths.push_back(current_input_length);
					    ref_ids.push_back(*sid);					
					    oneBests.push_back(bestModel);
					  }  
					}					
				}
				if (hope_model) {
					// HOPE
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << hope_n << "best hope translations" << endl;
					vector< vector<const Word*> > outputHope = decoder->getNBest(input, *sid, hope_n, 1.0, bleuWeight_hope,
							featureValuesHope[batchPosition], bleuScoresHope[batchPosition], modelScoresHope[batchPosition],
							1, distinctNbest, avgRefLength, rank, epoch, "");
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
							1, distinctNbest, avgRefLength, rank, epoch, "");
					bestModel = outputModel[0];
					decoder->cleanup(chartDecoding);
					cerr << endl;

					// needed for history
					if (historyBleu) {
						inputLengths.push_back(current_input_length);
						ref_ids.push_back(*sid);
						oneBests.push_back(bestModel);
					}						
					
					examples_in_batch++;
				}
				if (rank_only) {
					// MODEL
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << rank_n << "best wrt model score" << endl;
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, rank_n, 0.0, bleuWeight,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, avgRefLength, rank, epoch, "");
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
					ostringstream hope_nbest_filename, fear_nbest_filename, model_nbest_filename, ref_filename;
					if (sample && external_score) {					  
					  hope_nbest_filename << "decode_hope_rank" << rank << "." << hope_n << "best";
					  fear_nbest_filename << "decode_fear_rank" << rank << "." << fear_n << "best";
					  model_nbest_filename << "decode_model_rank" << rank << "." << n << "best";
					  
					  // save reference
					  ref_filename <<  "decode_ref_rank" << rank;
					  referenceFileMegam = ref_filename.str();
					  ofstream ref_out(referenceFileMegam.c_str());
					  if (!ref_out) {
					    ostringstream msg;
					    msg << "Unable to open " << referenceFileMegam;
					    throw runtime_error(msg.str());
					  }
					  ref_out << referenceSentences[decoder->getShortestReferenceIndex(*sid)][*sid] << "\n";
					  ref_out.close();
					}					
					
					// HOPE
					if (clear_static) {
					  delete decoder;
					  StaticData::ClearDataStatic();
					  decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
					  decoder->setBleuParameters(sentenceBleu, scaleByInputLength, scaleByAvgInputLength, scaleByInverseLength, scaleByAvgInverseLength, scaleByX, historySmoothing, bleu_smoothing_scheme);
					  decoder->setWeights(mosesWeights);
					}

					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best hope translations" << endl;
					size_t oraclePos = featureValues[batchPosition].size();
					vector <vector<const Word*> > outputHope = decoder->getNBest(input, *sid, n, 1.0, bleuWeight_hope,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, avgRefLength, rank, epoch, hope_nbest_filename.str());
					vector<const Word*> oracle = outputHope[0];
					// needed for history
					inputLengths.push_back(current_input_length);
					ref_ids.push_back(*sid);
					decoder->cleanup(chartDecoding);
					ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
					//float hope_length_ratio = (float)oracle.size()/ref_length;
					cerr << endl;
					
					oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
					oracleBleuScores.push_back(bleuScores[batchPosition][oraclePos]);
					oracleModelScores.push_back(modelScores[batchPosition][oraclePos]);

					// MODEL
					if (clear_static) {
					  delete decoder;
					  StaticData::ClearDataStatic();
					  decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
					  decoder->setBleuParameters(sentenceBleu, scaleByInputLength, scaleByAvgInputLength, scaleByInverseLength, scaleByAvgInverseLength, scaleByX, historySmoothing, bleu_smoothing_scheme);
					  decoder->setWeights(mosesWeights);
					}

					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best wrt model score" << endl;
					vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, n, 0.0, bleuWeight,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1,	distinctNbest, avgRefLength, rank, epoch, model_nbest_filename.str());
					vector<const Word*> bestModel = outputModel[0];
					decoder->cleanup(chartDecoding);
					oneBests.push_back(bestModel);
					ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
					//float model_length_ratio = (float)bestModel.size()/ref_length;
					cerr << endl;

					// FEAR
					if (clear_static) {
                                          delete decoder;
					  StaticData::ClearDataStatic();
                                          decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
                                          decoder->setBleuParameters(sentenceBleu, scaleByInputLength, scaleByAvgInputLength, scaleByInverseLength, scaleByAvgInverseLength, scaleByX, historySmoothing, bleu_smoothing_scheme);
                                          decoder->setWeights(mosesWeights);
					}
					
					cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best fear translations" << endl;
					vector< vector<const Word*> > outputFear = decoder->getNBest(input, *sid, n, -1.0, bleuWeight_fear,
							featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
							1, distinctNbest, avgRefLength, rank, epoch, fear_nbest_filename.str());
					vector<const Word*> fear = outputFear[0];
					decoder->cleanup(chartDecoding);
					ref_length = decoder->getClosestReferenceLength(*sid, fear.size());
					//float fear_length_ratio = (float)fear.size()/ref_length;
					for (size_t i = 0; i < fear.size(); ++i) {
						delete fear[i];
					}

					examples_in_batch++;
					
					if (sample) {
					  float bleuHope = -1000;
					  float bleuFear = 1000;
					  size_t indexHope = -1;
					  size_t indexFear = -1;
					  vector<float> bleuHopeList;
					  vector<float> bleuFearList;
					  vector<float> indexHopeList;
					  vector<float> indexFearList;
					  
					  if (external_score) {
					    // concatenate nbest files (use hope, model, fear lists to extract samples from)
					    stringstream nbestStreamMegam, catCmd, sortCmd, scoreDataFile, featureDataFile;
					    nbestStreamMegam << "decode_hypos_rank" << rank << "." << (hope_n+n+fear_n) << "best";
					    nbestFileMegam = nbestStreamMegam.str();
					    catCmd << "cat " << hope_nbest_filename.str() << " " << model_nbest_filename.str() 
						   << " " << fear_nbest_filename.str() << " > " << nbestFileMegam;
					    system(catCmd.str().c_str());
					    
					    // extract features and scores
					    scoreDataFile << "decode_hypos_rank" << rank << ".scores.dat";
					    featureDataFile << "decode_hypos_rank" << rank << ".features.dat";
					    stringstream extractorCmd;
					    extractorCmd << moses_src << "/dist/bin/extractor"
					      " --scconfig case:true --scfile " << scoreDataFile.str() << " --ffile " << featureDataFile.str() << " -r " << referenceFileMegam << " -n " << nbestFileMegam;
					    system(extractorCmd.str().c_str());
					    
					    // NOTE: here we are just scoring the nbest lists created above. 
					    // We will use the (real, not dynamically computed) sentence bleu scores to select a pair of two
					    // translations with maximal Bleu difference
					    vector<float> bleuScoresNbest = BleuScorer::ScoreNbestList(scoreDataFile.str(), featureDataFile.str());
					    for (size_t i=0; i < bleuScoresNbest.size(); ++i) {
					      //cerr << "bleu: " << bleuScoresNbest[i]*current_input_length << endl;
					      if (abs(bleuScoresNbest[i] - bleuHope) < epsilon) { // equal bleu scores
						if (modelScores[batchPosition][i] > modelScores[batchPosition][indexHope]) {
						  if (abs(modelScores[batchPosition][i] - modelScores[batchPosition][indexHope]) > epsilon) {
						    bleuHope = bleuScoresNbest[i];
						    indexHope = i;
						  }
						}
					      }
					      else if (bleuScoresNbest[i] > bleuHope) { // better than current best
						bleuHope = bleuScoresNbest[i];
						indexHope = i;
					      }
					      
					      if (abs(bleuScoresNbest[i] - bleuFear) < epsilon) { // equal bleu scores
						if (modelScores[batchPosition][i] > modelScores[batchPosition][indexFear]) {
						  if (abs(modelScores[batchPosition][i] - modelScores[batchPosition][indexFear]) > epsilon) {
						    bleuFear = bleuScoresNbest[i];
						    indexFear = i;
						  }
						}
					      }
					      else if (bleuScoresNbest[i] < bleuFear) { // worse than current worst
						bleuFear = bleuScoresNbest[i];
						indexFear = i;
					      }
					    }
					  }
					  else {
					    cerr << endl;
					    if (most_violated || all_violated || one_against_all) {
					      bleuHope = -1000;
					      bleuFear = 1000;
					      indexHope = -1;
					      indexFear = -1;
					      if (most_violated)
						cerr << "Rank " << rank << ", epoch " << epoch << ", pick pair with most violated constraint";
					      else if (all_violated)
						cerr << "Rank " << rank << ", epoch " << epoch << ", pick all pairs with violated constraints";
					      else 
						cerr << "Rank " << rank << ", epoch " << epoch << ", pick all pairs with hope";

					      // find best hope, then find fear that violates our constraint most
					      for (size_t i=0; i<bleuScores[batchPosition].size(); ++i) {
						if (abs(bleuScores[batchPosition][i] - bleuHope) < epsilon) { // equal bleu scores          
                                                  if (modelScores[batchPosition][i] > modelScores[batchPosition][indexHope]) {
                                                    if (abs(modelScores[batchPosition][i] - modelScores[batchPosition][indexHope]) > epsilon) {
						      // better model score
						      bleuHope = bleuScores[batchPosition][i];
						      indexHope = i;
						    }
                                                  }
                                                }
                                                else if (bleuScores[batchPosition][i] > bleuHope) { // better than current best         
                                                  bleuHope = bleuScores[batchPosition][i];
                                                  indexHope = i;
                                                }
					      }
			      
					      float currentViolation = 0;
					      float minimum_bleu_diff = 0.01;
					      for (size_t i=0; i<bleuScores[batchPosition].size(); ++i) {
						float bleuDiff = bleuHope - bleuScores[batchPosition][i];
						float modelDiff = modelScores[batchPosition][indexHope] - modelScores[batchPosition][i];
						if (bleuDiff > epsilon) {
						  if (one_against_all && bleuDiff > minimum_bleu_diff) {
						    cerr << ".. adding pair";
						    bleuHopeList.push_back(bleuHope);
						    bleuFearList.push_back(bleuScores[batchPosition][i]);
						    indexHopeList.push_back(indexHope);
						    indexFearList.push_back(i);
						  }
						  else if (modelDiff < bleuDiff) {
						    float diff = bleuDiff - modelDiff;
						    if (diff > epsilon) { 
						      if (all_violated) {
							cerr << ".. adding pair";
							bleuHopeList.push_back(bleuHope);
							bleuFearList.push_back(bleuScores[batchPosition][i]);
							indexHopeList.push_back(indexHope);
							indexFearList.push_back(i);
						      }
						      else if (most_violated && diff > currentViolation) {
							currentViolation = diff;
							bleuFear = bleuScores[batchPosition][i];
							indexFear = i;
							cerr << "Rank " << rank << ", epoch " << epoch << ", current violation: " << currentViolation << " (" << modelDiff << " >= " << bleuDiff << ")" << endl;
						      }						    
						    }
						  }
						}						
					      }
					      
					      if (most_violated) {
						if (currentViolation > 0) {
						  cerr << ".. adding pair with violation " << currentViolation << endl;
						  bleuHopeList.push_back(bleuHope);
						  bleuFearList.push_back(bleuFear);
						  indexHopeList.push_back(indexHope);
						  indexFearList.push_back(indexFear);
						}
						else cerr << ".. none" << endl;
					      }
					      else cerr << endl;
					    }
					    if (max_bleu_diff) {
					      bleuHope = -1000;
                                              bleuFear = 1000;
                                              indexHope = -1;
                                              indexFear = -1;
					      cerr << "Rank " << rank << ", epoch " << epoch << ", pick pair with max Bleu diff";
					      // use dynamically calculated scores to find best and worst 
					      for (size_t i=0; i<bleuScores[batchPosition].size(); ++i) {
						//cerr << "bleu: " << bleuScores[batchPosition][i] << endl;
						if (abs(bleuScores[batchPosition][i] - bleuHope) < epsilon) { // equal bleu scores
						  if (modelScores[batchPosition][i] > modelScores[batchPosition][indexHope]) {
						    if (abs(modelScores[batchPosition][i] - modelScores[batchPosition][indexHope]) > epsilon) {
						      bleuHope = bleuScores[batchPosition][i];
						      indexHope = i;
						    }
						  }
						}
						else if (bleuScores[batchPosition][i] > bleuHope) { // better than current best
						  bleuHope = bleuScores[batchPosition][i];
						  indexHope = i;
						}
						
						if (abs(bleuScores[batchPosition][i] - bleuFear) < epsilon) { // equal bleu scores
						  if (modelScores[batchPosition][i] > modelScores[batchPosition][indexFear]) {
						    if (abs(modelScores[batchPosition][i] - modelScores[batchPosition][indexFear]) > epsilon) {
						      bleuFear = bleuScores[batchPosition][i];
						      indexFear = i;
						    }
						  }
						}
						else if (bleuScores[batchPosition][i] < bleuFear) { // worse than current worst
						  bleuFear = bleuScores[batchPosition][i];
						  indexFear = i;
						}						      	
					      }
					      
					      if (bleuHope != -1000 && bleuFear != 1000 && (bleuHope - bleuFear) > epsilon) {
						cerr << ".. adding 1 pair" << endl;
						bleuHopeList.push_back(bleuHope);
						bleuFearList.push_back(bleuFear);
						indexHopeList.push_back(indexHope);
						indexFearList.push_back(indexFear);					      
					      }
					      else cerr << "none" << endl;
					    }
					  }
					  
					  if (bleuHopeList.size() == 0 || bleuFearList.size() == 0) {
					    cerr << "Rank " << rank << ", epoch " << epoch << ", no appropriate hypotheses found.." << endl;
					    skip_sample = true;
					  }
					  else {
					    if (bleuHope != -1000 && bleuFear != 1000 && bleuHope <= bleuFear) {
					      if (abs(bleuHope - bleuFear) < epsilon) 
						cerr << "\nRank " << rank << ", epoch " << epoch << ", WARNING: HOPE and FEAR have equal Bleu." << endl;
					      else 
						cerr << "\nRank " << rank << ", epoch " << epoch << ", ERROR: FEAR has better Bleu than HOPE." << endl;     				     
					    }
					    else {
					      if (external_score) {
						// use actual sentence bleu (not dynamically computed)
						bleuScoresHopeSample[batchPosition].push_back(bleuHope*current_input_length);
						bleuScoresFearSample[batchPosition].push_back(bleuFear*current_input_length);
						featureValuesHopeSample[batchPosition].push_back(featureValues[batchPosition][indexHope]);
						featureValuesFearSample[batchPosition].push_back(featureValues[batchPosition][indexFear]);
						modelScoresHopeSample[batchPosition].push_back(modelScores[batchPosition][indexHope]);
						modelScoresFearSample[batchPosition].push_back(modelScores[batchPosition][indexFear]);
						cerr << "Rank " << rank << ", epoch " << epoch << ", Best: " << bleuHope*current_input_length << " (" << indexHope << ")" << endl;
						cerr << "Rank " << rank << ", epoch " << epoch << ", Worst: " << bleuFear*current_input_length << " (" << indexFear << ")" << endl;
					      }
					      else {
						cerr << endl;
						for (size_t i=0; i<bleuHopeList.size(); ++i) {
						  float bHope = bleuHopeList[i];
						  float bFear = bleuFearList[i];
						  size_t iHope = indexHopeList[i];
						  size_t iFear = indexFearList[i];
						  cerr << "Rank " << rank << ", epoch " << epoch << ", Hope[" << i << "]: " << bHope << " (" << iHope << ")" << endl;
						  cerr << "Rank " << rank << ", epoch " << epoch << ", Fear[" << i << "]: " << bFear << " (" << iFear << ")" << endl;				
						  bleuScoresHopeSample[batchPosition].push_back(bHope);
						  bleuScoresFearSample[batchPosition].push_back(bFear);
						  featureValuesHopeSample[batchPosition].push_back(featureValues[batchPosition][iHope]);
						  featureValuesFearSample[batchPosition].push_back(featureValues[batchPosition][iFear]);
						  modelScoresHopeSample[batchPosition].push_back(modelScores[batchPosition][iHope]);
						  modelScoresFearSample[batchPosition].push_back(modelScores[batchPosition][iFear]);

						  featureValues[batchPosition][iHope].IncrementSparseHopeFeatures();
						  featureValues[batchPosition][iFear].IncrementSparseFearFeatures();
						}
					      }
					    }						
					  }
					}
				}

				// next input sentence
				++sid;
				++actualBatchSize;
				++shardPosition;
			} // end of batch loop

			if (megam) {
				// extract features and scores
				string scoreDataFile = "decode_hope-model-fear.scores.dat";
				string featureDataFile = "decode_hope-model-fear.features.dat";
				stringstream extractorCmd;
				//extractorCmd << "/afs/inf.ed.ac.uk/user/s07/s0787953/mosesdecoder_github_saxnot/dist/bin/extractor"
				extractorCmd << "/home/eva/mosesdecoder_github_saxnot/dist/bin/extractor"
						" --scconfig case:true --scfile " << scoreDataFile << " --ffile " << featureDataFile <<
						" -r " << referenceFileMegam << " -n " << nbestFileMegam;
				system(extractorCmd.str().c_str());
				
				/*
				// pro --> select training examples
				stringstream proCmd;
				string proDataFile = "decode_hope-fear.pro.data";
				//proCmd << "/afs/inf.ed.ac.uk/user/s07/s0787953/mosesdecoder_github_saxnot/dist/bin/pro"
				proCmd << "/home/eva/mosesdecoder_github_saxnot/dist/bin/pro"
						" --ffile " << featureDataFile << " --scfile " << scoreDataFile << " -o " << proDataFile;
				system(proCmd.str().c_str());

				// megam
				stringstream megamCmd;
				string megamOut = "decode_hope-fear.megam-weights";
				string megamErr = "decode_hope-fear.megam-log";
				//megamCmd << "/afs/inf.ed.ac.uk/user/s07/s0787953/mosesdecoder_github_saxnot/mert/megam_i686.opt"
				megamCmd << "/home/eva/mosesdecoder_github_saxnot/mert/megam_i686.opt"
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
		    //foreach (keys %{$sparse_weights}) { $$sparse_weights{$_} /= $sum; } */

			}
			else if (examples_in_batch == 0 || (sample && skip_sample)) {
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
				vector<const ScoreProducer*>::const_iterator iter;
				const vector<const ScoreProducer*> featureFunctions2 = staticData.GetTranslationSystem(TranslationSystem::DEFAULT).GetFeatureFunctions();
				for (iter = featureFunctions2.begin(); iter != featureFunctions2.end(); ++iter) {
				  if ((*iter)->GetScoreProducerWeightShortName() == "bl") {
				    mosesWeights.Assign(*iter, 0);
				    break;
				  }
				}

				// scale LM feature (to avoid rapid changes)
				if (scale_lm) {
				  const LMList& lmList_new = staticData.GetLMList();
				  for (LMList::const_iterator iter = lmList_new.begin(); iter != lmList_new.end(); ++iter) {
				    // scale up weight
				    float lmWeight = mosesWeights.GetScoreForProducer(*iter);
				    mosesWeights.Assign(*iter, lmWeight*scale_lm_factor);
				    cerr << "Rank " << rank << ", epoch " << epoch << ", lm weight scaled from " << lmWeight << " to " << lmWeight*scale_lm_factor << endl;

				    // scale down score
				    if (sample) {
				    	scaleFeatureScore(*iter, scale_lm_factor, featureValuesHopeSample, rank, epoch);
				    	scaleFeatureScore(*iter, scale_lm_factor, featureValuesFearSample, rank, epoch);
				    }
				    else {
				    	scaleFeatureScore(*iter, scale_lm_factor, featureValuesHope, rank, epoch);
				    	scaleFeatureScore(*iter, scale_lm_factor, featureValuesFear, rank, epoch);
				    	scaleFeatureScore(*iter, scale_lm_factor, featureValues, rank, epoch);
				    }
				  }
				}

				// scale WP
				if (scale_wp) {
				  // scale up weight  
				  WordPenaltyProducer *wp = staticData.GetFirstWordPenaltyProducer();
				  float wpWeight = mosesWeights.GetScoreForProducer(wp);
				  mosesWeights.Assign(wp, wpWeight*scale_wp_factor);
				  cerr << "Rank " << rank << ", epoch " << epoch << ", wp weight scaled from " << wpWeight << " to " << wpWeight*scale_wp_factor << endl;

				  // scale down score
				  if (sample) {
				    scaleFeatureScore(wp, scale_wp_factor, featureValuesHopeSample, rank, epoch);
				    scaleFeatureScore(wp, scale_wp_factor, featureValuesFearSample, rank, epoch);
				  }
				  else {
				    scaleFeatureScore(wp, scale_wp_factor, featureValuesHope, rank, epoch);
				    scaleFeatureScore(wp, scale_wp_factor, featureValuesFear, rank, epoch);
				    scaleFeatureScore(wp, scale_wp_factor, featureValues, rank, epoch);
				  }
				}

				if (scale_all) {
				  // scale distortion
				  DistortionScoreProducer *dp = staticData.GetDistortionScoreProducer();
				  float dWeight = mosesWeights.GetScoreForProducer(dp);
                                  mosesWeights.Assign(dp, dWeight*scale_all_factor);
                                  cerr << "Rank " << rank << ", epoch " << epoch << ", distortion weight scaled from " << dWeight << " to " << dWeight*scale_all_factor << endl;

                                  // scale down score                                                                                      
                                  if (sample) {
                                    scaleFeatureScore(dp, scale_all_factor, featureValuesHopeSample, rank, epoch);
                                    scaleFeatureScore(dp, scale_all_factor, featureValuesFearSample, rank, epoch);
                                  }
                                  else {
                                    scaleFeatureScore(dp, scale_all_factor, featureValuesHope, rank, epoch);
                                    scaleFeatureScore(dp, scale_all_factor, featureValuesFear, rank, epoch);
                                    scaleFeatureScore(dp, scale_all_factor, featureValues, rank, epoch);
                                  }

				  // scale lexical reordering models
				  vector<LexicalReordering*> lrVec = staticData.GetLexicalReorderModels();
                                  for (size_t i=0; i<lrVec.size(); ++i) {
				    LexicalReordering* lr = lrVec[i];
				    // scale up weight                                                                                    
				    vector<float> dWeights = mosesWeights.GetScoresForProducer(lr);
				    for (size_t j=0; j<dWeights.size(); ++j) {
				      cerr << "Rank " << rank << ", epoch " << epoch << ", d weight scaled from " << dWeights[j];
				      dWeights[j] *= scale_all_factor;
				      cerr << " to " << dWeights[j] << endl;
				    }
				    mosesWeights.Assign(lr, dWeights);
                           
				    // scale down score                                                                                  
				    if (sample) {
				      scaleFeatureScores(lr, scale_all_factor, featureValuesHopeSample, rank, epoch);
				      scaleFeatureScores(lr, scale_all_factor, featureValuesFearSample, rank, epoch);
				    }
				    else {
				      scaleFeatureScores(lr, scale_all_factor, featureValuesHope, rank, epoch);
				      scaleFeatureScores(lr, scale_all_factor, featureValuesFear, rank, epoch);
				      scaleFeatureScores(lr, scale_all_factor, featureValues, rank, epoch);
				    }
				  }
				  
				  // scale phrase table models
				  vector<PhraseDictionaryFeature*> pdVec = staticData.GetPhraseDictionaryModels();
                                  for (size_t i=0; i<pdVec.size(); ++i) {
				    PhraseDictionaryFeature* pd = pdVec[i];
                                    // scale up weight                                                                                        
                                    vector<float> tWeights = mosesWeights.GetScoresForProducer(pd);
				    for (size_t j=0; j<tWeights.size(); ++j) {
                                      cerr << "Rank " << rank << ", epoch " << epoch << ", t weight scaled from " << tWeights[j];
                                      tWeights[j] *= scale_all_factor;
                                      cerr << " to " << tWeights[j] << endl;
                                    }
                                    mosesWeights.Assign(pd, tWeights);

                                    // scale down score                                                                                     
                                    if (sample) {
                                      scaleFeatureScores(pd, scale_all_factor, featureValuesHopeSample, rank, epoch);
                                      scaleFeatureScores(pd, scale_all_factor, featureValuesFearSample, rank, epoch);
                                    }
                                    else {
                                      scaleFeatureScores(pd, scale_all_factor, featureValuesHope, rank, epoch);
                                      scaleFeatureScores(pd, scale_all_factor, featureValuesFear, rank, epoch);
                                      scaleFeatureScores(pd, scale_all_factor, featureValues, rank, epoch);
                                    }
                                  }
				}
				
				// set core features to 0 to avoid updating the feature weights
				if (coreWeightMap.size() > 0) {
					if (sample) {
						ignoreCoreFeatures(featureValuesHopeSample, coreWeightMap);
						ignoreCoreFeatures(featureValuesFearSample, coreWeightMap);
					}
					else {
						ignoreCoreFeatures(featureValues, coreWeightMap);
						ignoreCoreFeatures(featureValuesHope, coreWeightMap);
						ignoreCoreFeatures(featureValuesFear, coreWeightMap);
					}
				}
			
				// print out the feature values
				if (print_feature_values) {
					cerr << "\nRank " << rank << ", epoch " << epoch << ", feature values: " << endl;
					if (sample) {
						cerr << "hope: " << endl;
						printFeatureValues(featureValuesHopeSample);
						cerr << "fear: " << endl;
						printFeatureValues(featureValuesFearSample);
					}
					else if (model_hope_fear || rank_only) printFeatureValues(featureValues);
					else {
						cerr << "hope: " << endl;
						printFeatureValues(featureValuesHope);
						cerr << "fear: " << endl;
						printFeatureValues(featureValuesFear);
					}
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
					if (bleuScoresHope[0][0] >= min_oracle_bleu) {
						if (hope_n == 1 && fear_n ==1 && batchSize == 1) {
							update_status = ((MiraOptimiser*) optimiser)->updateWeightsAnalytically(mosesWeights, weightUpdate,
									featureValuesHope[0][0], featureValuesFear[0][0], bleuScoresHope[0][0], bleuScoresFear[0][0],
									modelScoresHope[0][0], modelScoresFear[0][0], learning_rate, rank, epoch);
						}
						else {
						  update_status = optimiser->updateWeightsHopeFear(mosesWeights, weightUpdate,
									featureValuesHope, featureValuesFear, bleuScoresHope, bleuScoresFear,
									modelScoresHope, modelScoresFear, learning_rate, rank, epoch);
						}
					}
					else
						update_status = 1;
				}
				else if (rank_only) {
					// learning ranking of model translations
					update_status = ((MiraOptimiser*) optimiser)->updateWeightsRankModel(mosesWeights, weightUpdate,
							featureValues, bleuScores, modelScores, learning_rate, rank, epoch);
				}
				else {
					// model_hope_fear
					if (sample) {
						update_status = optimiser->updateWeightsHopeFear(mosesWeights, weightUpdate,
								featureValuesHopeSample, featureValuesFearSample, bleuScoresHopeSample, bleuScoresFearSample,
								modelScoresHopeSample, modelScoresFearSample, learning_rate, rank, epoch);
					}
					else {
						update_status = ((MiraOptimiser*) optimiser)->updateWeights(mosesWeights, weightUpdate,
							featureValues, losses, bleuScores, modelScores, oracleFeatureValues, oracleBleuScores, oracleModelScores, learning_rate, rank, epoch);
					}
				}

//			sumStillViolatedConstraints += update_status;

				// rescale LM feature                                              
				if (scale_lm) {
				  const LMList& lmList_new = staticData.GetLMList();
				  for (LMList::const_iterator iter = lmList_new.begin(); iter != lmList_new.end(); ++iter) {
				    // scale weight back down                                                                
				    float lmWeight = mosesWeights.GetScoreForProducer(*iter);
				    mosesWeights.Assign(*iter, lmWeight/scale_lm_factor);
				    cerr << "Rank " << rank << ", epoch " << epoch << ", lm weight rescaled from " << lmWeight << " to " << lmWeight/scale_lm_factor << endl;
				  }
				}

				// rescale WP feature
				if (scale_wp) {
				  // scale weight back down
				  WordPenaltyProducer *wp = staticData.GetFirstWordPenaltyProducer();
				  float wpWeight = mosesWeights.GetScoreForProducer(wp);
				  mosesWeights.Assign(wp, wpWeight/scale_wp_factor);
				  cerr << "Rank " << rank << ", epoch " << epoch << ", wp weight rescaled from " << wpWeight << " to " << wpWeight/scale_wp_factor << endl;                                  
                                }

				if (scale_all) {
				  // rescale distortion
				  DistortionScoreProducer *dp = staticData.GetDistortionScoreProducer();
                                  float dWeight = mosesWeights.GetScoreForProducer(dp);
                                  mosesWeights.Assign(dp, dWeight/scale_all_factor);
                                  cerr << "Rank " << rank << ", epoch " << epoch << ", distortion weight rescaled from " << dWeight << " to " << dWeight/scale_all_factor << endl;

				  // rescale lexical reordering
				  vector<LexicalReordering*> lr = staticData.GetLexicalReorderModels();
                                  for (size_t i=0; i<lr.size(); ++i) {
				    vector<float> dWeights = mosesWeights.GetScoresForProducer(lr[i]);
				    for (size_t j=0; j<dWeights.size(); ++j) {
				      cerr << "Rank " << rank << ", epoch " << epoch << ", d weight rescaled from " << dWeights[j];
				      dWeights[j] /=scale_all_factor;
				      cerr << " to " << dWeights[j] << endl;				    
				    }
				    mosesWeights.Assign(lr[i], dWeights);				    
				  }

				  // rescale phrase models
				  vector<PhraseDictionaryFeature*> pd = staticData.GetPhraseDictionaryModels();
                                  for (size_t i=0; i<pd.size(); ++i) {
				    vector<float> tWeights = mosesWeights.GetScoresForProducer(pd[i]);
				    for (size_t j=0; j<tWeights.size(); ++j) {
                                      cerr << "Rank " << rank << ", epoch " << epoch << ", t weight rescaled from " << tWeights[j];
                                      tWeights[j] /=scale_all_factor;
                                      cerr << " to " << tWeights[j] << endl;
                                    }
                                    mosesWeights.Assign(pd[i], tWeights);
				  }
				}

				if (update_status == 0) {	 // if weights were updated
					// apply weight update
				        cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << weightUpdate << endl;
					
					if (l2_regularize) {
					  weightUpdate.L2Regularize(l2_lambda);
					  cerr << "Rank " << rank << ", epoch " << epoch << ", " 
					       << "l2-reg. on mosesWeights with lambda=" << l2_lambda << endl;  
					  cerr << "Rank " << rank << ", epoch " << epoch << ", regularized update: " << weightUpdate << endl;
					}

					if (feature_confidence) {
					  cerr << "Rank " << rank << ", epoch " << epoch << ", apply feature learning rates with decay " << decay << ": " << featureLearningRates << endl;
					  weightUpdate.MultiplyEqualsSafe(featureLearningRates);
					  if (rank == 0)
					    cerr << "Rank " << rank << ", epoch " << epoch << ", scaled update: " << weightUpdate << endl;
					  
					  // update confidence counts
					  confidenceCounts.UpdateConfidenceCounts(weightUpdate, signed_counts);
					  
					  // update feature learning rates
					  featureLearningRates.UpdateLearningRates(decay, confidenceCounts);
					}
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

					// adjust bleu weight
					if (bleu_weight_lm_adjust) {
					  float lmSum = 0;
					  const LMList& lmList_new = staticData.GetLMList();
					  for (LMList::const_iterator i = lmList_new.begin(); i != lmList_new.end(); ++i)
					    lmSum += abs(mosesWeights.GetScoreForProducer(*i));
					  bleuWeight = lmSum * bleu_weight_lm_factor;
					  cerr << "Rank " << rank << ", epoch " << epoch << ", adjusting Bleu weight to " << bleuWeight << " (factor " << bleu_weight_lm_factor << ")" << endl;
					  
					  if (bleuWeight_hope == -1) {
					    bleuWeight_hope = bleuWeight;
					  }
					  if (bleuWeight_fear == -1) {
					    bleuWeight_fear = bleuWeight;
					  }
					}
				}

				// update history (for approximate document Bleu)
				if (historyBleu) {
					for (size_t i = 0; i < oneBests.size(); ++i) 
						cerr << "Rank " << rank << ", epoch " << epoch << ", update history with 1best length: " << oneBests[i].size() << " ";
					decoder->updateHistory(oneBests, inputLengths, ref_ids, rank, epoch);
				}
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

				// mix confidence counts
				mpi::reduce(world, confidenceCounts, mixedConfidenceCounts, SCCPlus(), 0);
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
					
					// divide confidence counts 
					mixedConfidenceCounts.DivideEquals(size);

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
					
					if (l1_regularize && weightMixingThisEpoch == mixingFrequency) {
					  mixedWeights.L1Regularize(l1_lambda);
					  cerr << "Rank " << rank << ", epoch " << epoch << ", " 
					       << "l1-reg. on mixedWeights with lambda=" << l1_lambda << endl;
					  
					  // subtract lambda from every weight in the sum --> multiply by number of updates
					  cumulativeWeights.L1Regularize(l1_lambda*numberOfUpdatesThisEpoch);
					  cerr << "Rank " << rank << ", epoch " << epoch << ", " 
					       << "l1-reg. on cumulativeWeights with lambda=" << l1_lambda*numberOfUpdatesThisEpoch << endl;
					}										
				}

				// broadcast average weights from process 0
				mpi::broadcast(world, mixedWeights, 0);
				decoder->setWeights(mixedWeights);
				mosesWeights = mixedWeights;

				// broadcast confidence counts
				if (rank == 0)
				  cerr << "Rank " << rank << ", epoch " << epoch << ", confidence counts before: " << confidenceCounts << endl;
				mpi::broadcast(world, mixedConfidenceCounts, 0);
				confidenceCounts = mixedConfidenceCounts;
				if (rank == 0)
                                  cerr << "Rank " << rank << ", epoch " << epoch << ", confidence counts after: " << confidenceCounts << endl;
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

			      	mixedAverageWeights.PrintSparseHopeFeatureCounts(sparseFeatureCountsHope);
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

		if (historyBleu) {
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
					//cerr << "insert 1 weight for " << featureFunctions[i]->GetScoreProducerDescription();
					//cerr << " (" << weight << ")" << endl;
				}
				else {
					store_weights.push_back(weight);
					if (store_weights.size() == featureFunctions[i]->GetNumScoreComponents()) {
						coreWeightMap.insert(ProducerWeightPair(featureFunctions[i], store_weights));
						/*cerr << "insert " << store_weights.size() << " weights for " << featureFunctions[i]->GetScoreProducerDescription() << " (";
						for (size_t j=0; j < store_weights.size(); ++j)
							cerr << store_weights[j] << " ";
							cerr << ")" << endl;*/
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

void deleteTranslations(vector<vector<const Word*> > &translations) {
	for (size_t i = 0; i < translations.size(); ++i) {
		for (size_t j = 0; j < translations[i].size(); ++j) {
			delete translations[i][j];
		}
	}
}

void decodeHopeOrFear(size_t rank, size_t size, size_t decode, string filename, vector<string> &inputSentences, MosesDecoder* decoder, size_t n, float bleuWeight) {
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
		vector< vector<const Word*> > nbestOutput = decoder->getNBest(input, sid, n, factor, bleuWeight, dummyFeatureValues[0],
				dummyBleuScores[0], dummyModelScores[0], n, true, false, rank, 0, "");
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

void scaleFeatureScore(ScoreProducer *sp, float scaling_factor, vector<vector<ScoreComponentCollection> > &featureValues, size_t rank, size_t epoch) {
  string name = sp->GetScoreProducerWeightShortName();

  // scale down score
  float featureScore;
  for (size_t i=0; i<featureValues.size(); ++i) { // each item in batch
    for (size_t j=0; j<featureValues[i].size(); ++j) { // each item in nbest
      featureScore = featureValues[i][j].GetScoreForProducer(sp);
      featureValues[i][j].Assign(sp, featureScore/scaling_factor);
      //cerr << "Rank " << rank << ", epoch " << epoch << ", " << name << " score scaled from " << featureScore << " to " << featureScore/scaling_factor << endl;
    }
  }
}

void scaleFeatureScores(ScoreProducer *sp, float scaling_factor, vector<vector<ScoreComponentCollection> > &featureValues, size_t rank, size_t epoch) {
  string name = sp->GetScoreProducerWeightShortName();

  // scale down score                                                                                                                         
  for (size_t i=0; i<featureValues.size(); ++i) { // each item in batch                                                                       
    for (size_t j=0; j<featureValues[i].size(); ++j) { // each item in nbest                                                               
      vector<float> featureScores = featureValues[i][j].GetScoresForProducer(sp);
      for (size_t k=0; k<featureScores.size(); ++k)
	featureScores[k] /= scaling_factor;
      featureValues[i][j].Assign(sp, featureScores);
      //cerr << "Rank " << rank << ", epoch " << epoch << ", " << name << " score scaled from " << featureScore << " to " << featureScore/scaling_factor << endl;                                                                                                                            
    }
  }
}
