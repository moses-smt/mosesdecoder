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
#include "Optimiser.h"
#include "Hildreth.h"
#include "HypothesisQueue.h"
#include "moses/StaticData.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/ThreadPool.h"
#include "mert/BleuScorer.h"
#include "moses/FeatureVector.h"

#include "moses/FF/WordTranslationFeature.h"
#include "moses/FF/PhrasePairFeature.h"
#include "moses/FF/WordPenaltyProducer.h"
#include "moses/LM/Base.h"
#include "util/random.hh"

using namespace Mira;
using namespace std;
using namespace Moses;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
  util::rand_init();
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
  //  string coreWeightFile, startWeightFile;
  size_t epochs;
  string learner;
  bool shuffle;
  size_t mixingFrequency;
  size_t weightDumpFrequency;
  string weightDumpStem;
  bool scale_margin;
  bool scale_update;
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
  bool hope_fear;
  bool model_hope_fear;
  size_t hope_n, fear_n;
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
  bool printFeatureCounts, printNbestWithFeatures;
  bool avgRefLength;
  bool print_weights, print_core_weights, debug_model, scale_lm, scale_wp;
  float scale_lm_factor, scale_wp_factor;
  bool kbest;
  string moses_src;
  float sigmoidParam;
  float bleuWeight, bleuWeight_hope, bleuWeight_fear;
  bool bleu_weight_lm;
  float bleu_weight_lm_factor;
  bool l1_regularize, l2_regularize, l1_reg_sparse, l2_reg_sparse;
  float l1_lambda, l2_lambda;
  bool most_violated, most_violated_reg, all_violated, max_bleu_diff;
  bool feature_confidence, signed_counts;
  float decay_core, decay_sparse, core_r0, sparse_r0;
  float bleu_weight_fear_factor;
  bool hildreth;
  float add2lm;

  // compute real sentence Bleu scores on complete translations, disable Bleu feature
  bool realBleu, disableBleuFeature;
  bool rescaleSlack;
  bool makePairs;
  bool debug;
  bool reg_on_every_mix;
  size_t continue_epoch;
  bool modelPlusBleu,  simpleHistoryBleu;
  po::options_description desc("Allowed options");
  desc.add_options()
  ("continue-epoch", po::value<size_t>(&continue_epoch)->default_value(0), "Continue an interrupted experiment from this epoch on")
  ("freq-reg", po::value<bool>(&reg_on_every_mix)->default_value(false), "Regularize after every weight mixing")
  ("l1sparse", po::value<bool>(&l1_reg_sparse)->default_value(true), "L1-regularization for sparse weights only")
  ("l2sparse", po::value<bool>(&l2_reg_sparse)->default_value(true), "L2-regularization for sparse weights only")
  ("mv-reg", po::value<bool>(&most_violated_reg)->default_value(false), "Regularize most violated constraint")
  ("most-violated", po::value<bool>(&most_violated)->default_value(false), "Add most violated constraint")
  ("all-violated", po::value<bool>(&all_violated)->default_value(false), "Add all violated constraints")
  ("feature-confidence", po::value<bool>(&feature_confidence)->default_value(false), "Confidence-weighted learning")
  ("signed-counts", po::value<bool>(&signed_counts)->default_value(false), "Use signed feature counts for CWL")
  ("dbg", po::value<bool>(&debug)->default_value(true), "More debug output")
  ("make-pairs", po::value<bool>(&makePairs)->default_value(true), "Make pairs of hypotheses for 1slack")
  ("debug", po::value<bool>(&debug)->default_value(true), "More debug output")
  ("rescale-slack", po::value<bool>(&rescaleSlack)->default_value(false), "Rescale slack in 1-slack formulation")
  ("add2lm", po::value<float>(&add2lm)->default_value(0.0), "Add the specified amount to all LM weights")
  ("hildreth", po::value<bool>(&hildreth)->default_value(false), "Prefer Hildreth over analytical update")
  ("model-plus-bleu", po::value<bool>(&modelPlusBleu)->default_value(false), "Use the sum of model score and +/- bleu to select hope and fear translations")
  ("simple-history-bleu", po::value<bool>(&simpleHistoryBleu)->default_value(false), "Simple history Bleu")

  ("bleu-weight", po::value<float>(&bleuWeight)->default_value(1.0), "Bleu weight used in decoder objective")
  ("bw-hope", po::value<float>(&bleuWeight_hope)->default_value(-1.0), "Bleu weight used in decoder objective for hope")
  ("bw-fear", po::value<float>(&bleuWeight_fear)->default_value(-1.0), "Bleu weight used in decoder objective for fear")

  ("core-r0", po::value<float>(&core_r0)->default_value(1.0), "Start learning rate for core features")
  ("sparse-r0", po::value<float>(&sparse_r0)->default_value(1.0), "Start learning rate for sparse features")
  ("decay-core", po::value<float>(&decay_core)->default_value(0.01), "Decay for core feature learning rate")
  ("decay-sparse", po::value<float>(&decay_sparse)->default_value(0.01), "Decay for sparse feature learning rate")

  ("tie-bw-to-lm", po::value<bool>(&bleu_weight_lm)->default_value(true), "Make bleu weight depend on lm weight")
  ("bw-lm-factor", po::value<float>(&bleu_weight_lm_factor)->default_value(2.0), "Make bleu weight depend on lm weight by this factor")
  ("bw-factor-fear", po::value<float>(&bleu_weight_fear_factor)->default_value(1.0), "Multiply fear weight by this factor")
  ("accumulate-weights", po::value<bool>(&accumulateWeights)->default_value(false), "Accumulate and average weights over all epochs")
  ("average-weights", po::value<bool>(&averageWeights)->default_value(false), "Set decoder weights to average weights after each update")
  ("avg-ref-length", po::value<bool>(&avgRefLength)->default_value(false), "Use average reference length instead of shortest for BLEU score feature")
  ("batch-equals-shard", po::value<bool>(&batchEqualsShard)->default_value(false), "Batch size is equal to shard size (purely batch)")
  ("batch-size,b", po::value<size_t>(&batchSize)->default_value(1), "Size of batch that is send to optimiser for weight adjustments")
  ("bleu-smoothing-scheme", po::value<size_t>(&bleu_smoothing_scheme)->default_value(1), "Set a smoothing scheme for sentence-Bleu: +1 (1), +0.1 (2), papineni (3) (default:1)")
  ("boost", po::value<bool>(&boost)->default_value(false), "Apply boosting factor to updates on misranked candidates")
  ("config,f", po::value<string>(&mosesConfigFile), "Moses ini-file")
  ("configs-folds", po::value<vector<string> >(&mosesConfigFilesFolds), "Moses ini-files, one for each fold")
  ("debug-model", po::value<bool>(&debug_model)->default_value(false), "Get best model translation for debugging purposes")
  ("decode-hope", po::value<bool>(&decode_hope)->default_value(false), "Decode dev input set according to hope objective")
  ("decode-fear", po::value<bool>(&decode_fear)->default_value(false), "Decode dev input set according to fear objective")
  ("decode-model", po::value<bool>(&decode_model)->default_value(false), "Decode dev input set according to normal objective")
  ("decode-filename", po::value<string>(&decode_filename), "Filename for Bleu objective translations")
  ("decoder-settings", po::value<string>(&decoder_settings)->default_value(""), "Decoder settings for tuning runs")
  ("distinct-nbest", po::value<bool>(&distinctNbest)->default_value(true), "Use n-best list with distinct translations in inference step")
  ("dump-mixed-weights", po::value<bool>(&dumpMixedWeights)->default_value(false), "Dump mixed weights instead of averaged weights")
  ("epochs,e", po::value<size_t>(&epochs)->default_value(10), "Number of epochs")
  ("feature-cutoff", po::value<int>(&featureCutoff)->default_value(-1), "Feature cutoff as additional regularization for sparse features")
  ("fear-n", po::value<size_t>(&fear_n)->default_value(1), "Number of fear translations used")
  ("help", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("history-bleu", po::value<bool>(&historyBleu)->default_value(false), "Use 1best translations to update the history")
  ("history-smoothing", po::value<float>(&historySmoothing)->default_value(0.9), "Adjust the factor for history smoothing")
  ("hope-fear", po::value<bool>(&hope_fear)->default_value(true), "Use only hope and fear translations for optimisation (not model)")
  ("hope-n", po::value<size_t>(&hope_n)->default_value(2), "Number of hope translations used")
  ("input-file,i", po::value<string>(&inputFile), "Input file containing tokenised source")
  ("input-files-folds", po::value<vector<string> >(&inputFilesFolds), "Input files containing tokenised source, one for each fold")
  ("learner,l", po::value<string>(&learner)->default_value("mira"), "Learning algorithm")
  ("l1-lambda", po::value<float>(&l1_lambda)->default_value(0.0001), "Lambda for l1-regularization (w_i +/- lambda)")
  ("l2-lambda", po::value<float>(&l2_lambda)->default_value(0.01), "Lambda for l2-regularization (w_i * (1 - lambda))")
  ("l1-reg", po::value<bool>(&l1_regularize)->default_value(false), "L1-regularization")
  ("l2-reg", po::value<bool>(&l2_regularize)->default_value(false), "L2-regularization")
  ("min-bleu-ratio", po::value<float>(&minBleuRatio)->default_value(-1), "Set a minimum BLEU ratio between hope and fear")
  ("max-bleu-ratio", po::value<float>(&maxBleuRatio)->default_value(-1), "Set a maximum BLEU ratio between hope and fear")
  ("max-bleu-diff", po::value<bool>(&max_bleu_diff)->default_value(true), "Select hope/fear with maximum Bleu difference")
  ("min-oracle-bleu", po::value<float>(&min_oracle_bleu)->default_value(0), "Set a minimum oracle BLEU score")
  ("min-weight-change", po::value<float>(&min_weight_change)->default_value(0.0001), "Set minimum weight change for stopping criterion")
  ("mira-learning-rate", po::value<float>(&mira_learning_rate)->default_value(1), "Learning rate for MIRA (fixed or flexible)")
  ("mixing-frequency", po::value<size_t>(&mixingFrequency)->default_value(10), "How often per epoch to mix weights, when using mpi")
  ("model-hope-fear", po::value<bool>(&model_hope_fear)->default_value(false), "Use model, hope and fear translations for optimisation")
  ("moses-src", po::value<string>(&moses_src)->default_value(""), "Moses source directory")
  ("nbest,n", po::value<size_t>(&n)->default_value(30), "Number of translations in n-best list")
  ("normalise-weights", po::value<bool>(&normaliseWeights)->default_value(false), "Whether to normalise the updated weights before passing them to the decoder")
  ("normalise-margin", po::value<bool>(&normaliseMargin)->default_value(false), "Normalise the margin: squash between 0 and 1")
  ("perceptron-learning-rate", po::value<float>(&perceptron_learning_rate)->default_value(0.01), "Perceptron learning rate")
  ("print-feature-values", po::value<bool>(&print_feature_values)->default_value(false), "Print out feature values")
  ("print-feature-counts", po::value<bool>(&printFeatureCounts)->default_value(false), "Print out feature values, print feature list with hope counts after 1st epoch")
  ("print-nbest-with-features", po::value<bool>(&printNbestWithFeatures)->default_value(false), "Print out feature values, print feature list with hope counts after 1st epoch")
  ("print-weights", po::value<bool>(&print_weights)->default_value(false), "Print out current weights")
  ("print-core-weights", po::value<bool>(&print_core_weights)->default_value(true), "Print out current core weights")
  ("prune-zero-weights", po::value<bool>(&pruneZeroWeights)->default_value(false), "Prune zero-valued sparse feature weights")
  ("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training")
  ("reference-files-folds", po::value<vector<string> >(&referenceFilesFolds), "Reference translation files for training, one for each fold")
  ("kbest", po::value<bool>(&kbest)->default_value(true), "Select hope/fear pairs from a list of nbest translations")

  ("scale-by-inverse-length", po::value<bool>(&scaleByInverseLength)->default_value(false), "Scale BLEU by (history of) inverse input length")
  ("scale-by-input-length", po::value<bool>(&scaleByInputLength)->default_value(true), "Scale BLEU by (history of) input length")
  ("scale-by-avg-input-length", po::value<bool>(&scaleByAvgInputLength)->default_value(false), "Scale BLEU by average input length")
  ("scale-by-avg-inverse-length", po::value<bool>(&scaleByAvgInverseLength)->default_value(false), "Scale BLEU by average inverse input length")
  ("scale-by-x", po::value<float>(&scaleByX)->default_value(0.1), "Scale the BLEU score by value x")
  ("scale-lm", po::value<bool>(&scale_lm)->default_value(true), "Scale the language model feature")
  ("scale-factor-lm", po::value<float>(&scale_lm_factor)->default_value(0.5), "Scale the language model feature by this factor")
  ("scale-wp", po::value<bool>(&scale_wp)->default_value(false), "Scale the word penalty feature")
  ("scale-factor-wp", po::value<float>(&scale_wp_factor)->default_value(2), "Scale the word penalty feature by this factor")
  ("scale-margin", po::value<bool>(&scale_margin)->default_value(0), "Scale the margin by the Bleu score of the oracle translation")
  ("sentence-level-bleu", po::value<bool>(&sentenceBleu)->default_value(true), "Use a sentences level Bleu scoring function")
  ("shuffle", po::value<bool>(&shuffle)->default_value(false), "Shuffle input sentences before processing")
  ("sigmoid-param", po::value<float>(&sigmoidParam)->default_value(1), "y=sigmoidParam is the axis that this sigmoid approaches")
  ("slack", po::value<float>(&slack)->default_value(0.05), "Use slack in optimiser")
  ("sparse-average", po::value<bool>(&sparseAverage)->default_value(false), "Average weights by the number of processes")
  ("sparse-no-average", po::value<bool>(&sparseNoAverage)->default_value(false), "Don't average sparse weights, just sum")
  ("stop-weights", po::value<bool>(&weightConvergence)->default_value(true), "Stop when weights converge")
  ("verbosity,v", po::value<int>(&verbosity)->default_value(0), "Verbosity level")
  ("weight-dump-frequency", po::value<size_t>(&weightDumpFrequency)->default_value(2), "How often per epoch to dump weights (mpi)")
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
  } else {
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
  } else {
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

  if (historyBleu || simpleHistoryBleu) {
    sentenceBleu = false;
    cerr << "Using history Bleu. " << endl;
  }

  if (kbest) {
    realBleu = true;
    disableBleuFeature = true;
    cerr << "Use kbest lists and real Bleu scores, disable Bleu feature.." << endl;
  }

  // initialise Moses
  // add references to initialize Bleu feature
  boost::trim(decoder_settings);
  decoder_settings += " -mira -n-best-list - " + boost::lexical_cast<string>(n) + " distinct";

  vector<string> decoder_params;
  boost::split(decoder_params, decoder_settings, boost::is_any_of("\t "));

  // bleu feature
  decoder_params.push_back("-feature-add");

  decoder_settings = "BleuScoreFeature tuneable=false references=";
  if (trainWithMultipleFolds) {
    decoder_settings += referenceFilesFolds[myFold];
  } else {
    decoder_settings += referenceFiles[0];
    for (size_t i=1; i < referenceFiles.size(); ++i) {
      decoder_settings += ",";
      decoder_settings += referenceFiles[i];
    }
  }
  decoder_params.push_back(decoder_settings);

  string configFile = trainWithMultipleFolds? mosesConfigFilesFolds[myFold] : mosesConfigFile;
  VERBOSE(1, "Rank " << rank << " reading config file from " << configFile << endl);
  MosesDecoder* decoder = new MosesDecoder(configFile, verbosity, decoder_params.size(), decoder_params);
  decoder->setBleuParameters(disableBleuFeature, sentenceBleu, scaleByInputLength, scaleByAvgInputLength,
                             scaleByInverseLength, scaleByAvgInverseLength,
                             scaleByX, historySmoothing, bleu_smoothing_scheme, simpleHistoryBleu);
  bool chartDecoding = staticData.IsChart();

  // Optionally shuffle the sentences
  vector<size_t> order;
  if (trainWithMultipleFolds) {
    for (size_t i = 0; i < inputSentencesFolds[myFold].size(); ++i) {
      order.push_back(i);
    }
  } else {
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
    optimiser = new MiraOptimiser(slack, scale_margin, scale_update, boost, normaliseMargin, sigmoidParam);
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

  if (hope_n == 0)
    hope_n = n;
  if (fear_n == 0)
    fear_n = n;

  if (model_hope_fear || kbest)
    hope_fear = false; // is true by default
  if (learner == "mira" && !(hope_fear || model_hope_fear || kbest)) {
    cerr << "Error: Need to select one of parameters --hope-fear/--model-hope-fear/--kbest for mira update." << endl;
    return 1;
  }

#ifdef MPI_ENABLE
  if (!trainWithMultipleFolds)
    mpi::broadcast(world, order, 0);
#endif

  // Create shards according to the number of processes used
  vector<size_t> shard;
  if (trainWithMultipleFolds) {
    size_t shardSize = order.size()/coresPerFold;
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
  } else {
    size_t shardSize = order.size() / size;
    size_t shardStart = (size_t) (shardSize * rank);
    size_t shardEnd = (size_t) (shardSize * (rank + 1));
    if (rank == size - 1) {
      shardEnd = order.size();
      shardSize = shardEnd - shardStart;
    }
    VERBOSE(1, "Rank: " << rank << " Shard size: " << shardSize << endl);
    VERBOSE(1, "Rank: " << rank << " Shard start: " << shardStart << " Shard end: " << shardEnd << endl);
    shard.resize(shardSize);
    copy(order.begin() + shardStart, order.begin() + shardEnd, shard.begin());
    if (batchEqualsShard)
      batchSize = shardSize;
  }

  // get reference to feature functions
  // const vector<FeatureFunction*> &featureFunctions = FeatureFunction::GetFeatureFunctions();
  ScoreComponentCollection initialWeights = decoder->getWeights();

  if (add2lm != 0) {
    const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
    for (size_t i = 0; i < statefulFFs.size(); ++i) {
      const StatefulFeatureFunction *ff = statefulFFs[i];
      const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff);

      if (lm) {
        float lmWeight = initialWeights.GetScoreForProducer(lm) + add2lm;
        initialWeights.Assign(lm, lmWeight);
        cerr << "Rank " << rank << ", add " << add2lm << " to lm weight." << endl;
      }
    }
  }

  if (normaliseWeights) {
    initialWeights.L1Normalise();
    cerr << "Rank " << rank << ", normalised initial weights: " << initialWeights << endl;
  }

  decoder->setWeights(initialWeights);

  // set bleu weight to twice the size of the language model weight(s)
  if (bleu_weight_lm) {
    float lmSum = 0;
    const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
    for (size_t i = 0; i < statefulFFs.size(); ++i) {
      const StatefulFeatureFunction *ff = statefulFFs[i];
      const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff);

      if (lm) {
        lmSum += abs(initialWeights.GetScoreForProducer(lm));
      }
    }

    bleuWeight = lmSum * bleu_weight_lm_factor;
    if (!kbest) cerr << "Set bleu weight to lm weight * " << bleu_weight_lm_factor << endl;
  }

  // bleu weights can be set separately for hope and fear; otherwise they are both set to 'lm weight * bleu_weight_lm_factor'
  if (bleuWeight_hope == -1) {
    bleuWeight_hope = bleuWeight;
  }
  if (bleuWeight_fear == -1) {
    bleuWeight_fear = bleuWeight;
  }
  bleuWeight_fear *= bleu_weight_fear_factor;
  if (!kbest) {
    cerr << "Bleu weight: " << bleuWeight << endl;
    cerr << "Bleu weight fear: " << bleuWeight_fear << endl;
  }

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

  bool stop = false;
//	int sumStillViolatedConstraints;
  float epsilon = 0.0001;

  // Variables for feature confidence
  ScoreComponentCollection confidenceCounts, mixedConfidenceCounts, featureLearningRates;
  featureLearningRates.UpdateLearningRates(decay_core, decay_sparse, confidenceCounts, core_r0, sparse_r0); //initialise core learning rates
  cerr << "Initial learning rates, core: " << core_r0 << ", sparse: " << sparse_r0 << endl;

  for (size_t epoch = continue_epoch; epoch < epochs && !stop; ++epoch) {
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
        size_t shardSize = order.size()/coresPerFold;
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
      } else {
        size_t shardSize = order.size()/size;
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
      vector<vector<ScoreComponentCollection> > featureValuesHope;
      vector<vector<ScoreComponentCollection> > featureValuesFear;
      vector<vector<float> > bleuScoresHope;
      vector<vector<float> > bleuScoresFear;
      vector<vector<float> > modelScoresHope;
      vector<vector<float> > modelScoresFear;

      // get moses weights
      ScoreComponentCollection mosesWeights = decoder->getWeights();
      VERBOSE(1, "\nRank " << rank << ", epoch " << epoch << ", weights: " << mosesWeights << endl);

      if (historyBleu || simpleHistoryBleu) {
        decoder->printBleuFeatureHistory(cerr);
      }

      // BATCHING: produce nbest lists for all input sentences in batch
      vector<float> oracleBleuScores;
      vector<float> oracleModelScores;
      vector<vector<const Word*> > oneBests;
      vector<ScoreComponentCollection> oracleFeatureValues;
      vector<size_t> inputLengths;
      vector<size_t> ref_ids;
      size_t actualBatchSize = 0;

      size_t examples_in_batch = 0;
      bool skip_example = false;
      for (size_t batchPosition = 0; batchPosition < batchSize && sid
           != shard.end(); ++batchPosition) {
        string input;
        if (trainWithMultipleFolds)
          input = inputSentencesFolds[myFold][*sid];
        else
          input = inputSentences[*sid];

        Moses::Sentence *sentence = new Sentence();
        stringstream in(input + "\n");
        const vector<FactorType> inputFactorOrder = staticData.GetInputFactorOrder();
        sentence->Read(in,inputFactorOrder);
        cerr << "\nRank " << rank << ", epoch " << epoch << ", input sentence " << *sid << ": \"";
        sentence->Print(cerr);
        cerr << "\"" << " (batch pos " << batchPosition << ")" << endl;
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
        if (model_hope_fear) {
          featureValues.push_back(newFeatureValues);
          bleuScores.push_back(newScores);
          modelScores.push_back(newScores);
        }
        if (hope_fear || perceptron_update) {
          featureValuesHope.push_back(newFeatureValues);
          featureValuesFear.push_back(newFeatureValues);
          bleuScoresHope.push_back(newScores);
          bleuScoresFear.push_back(newScores);
          modelScoresHope.push_back(newScores);
          modelScoresFear.push_back(newScores);
          if (historyBleu || simpleHistoryBleu || debug_model) {
            featureValues.push_back(newFeatureValues);
            bleuScores.push_back(newScores);
            modelScores.push_back(newScores);
          }
        }
        if (kbest) {
          // for decoding
          featureValues.push_back(newFeatureValues);
          bleuScores.push_back(newScores);
          modelScores.push_back(newScores);

          // for storing selected examples
          featureValuesHope.push_back(newFeatureValues);
          featureValuesFear.push_back(newFeatureValues);
          bleuScoresHope.push_back(newScores);
          bleuScoresFear.push_back(newScores);
          modelScoresHope.push_back(newScores);
          modelScoresFear.push_back(newScores);
        }

        size_t ref_length;
        float avg_ref_length;

        if (print_weights)
          cerr << "Rank " << rank << ", epoch " << epoch << ", current weights: " << mosesWeights << endl;
        if (print_core_weights) {
          cerr << "Rank " << rank << ", epoch " << epoch << ", current weights: ";
          mosesWeights.PrintCoreFeatures();
          cerr << endl;
        }

        // check LM weight
        const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
        for (size_t i = 0; i < statefulFFs.size(); ++i) {
          const StatefulFeatureFunction *ff = statefulFFs[i];
          const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff);

          if (lm) {
            float lmWeight = mosesWeights.GetScoreForProducer(lm);
            cerr << "Rank " << rank << ", epoch " << epoch << ", lm weight: " << lmWeight << endl;
            if (lmWeight <= 0) {
              cerr << "Rank " << rank << ", epoch " << epoch << ", ERROR: language model weight should never be <= 0." << endl;
              mosesWeights.Assign(lm, 0.1);
              cerr << "Rank " << rank << ", epoch " << epoch << ", assign lm weights of 0.1" << endl;
            }
          }
        }

        // select inference scheme
        cerr << "Rank " << rank << ", epoch " << epoch << ", real Bleu? " << realBleu << endl;
        if (hope_fear || perceptron_update) {
          // HOPE
          cerr << "Rank " << rank << ", epoch " << epoch << ", " << hope_n <<
               "best hope translations" << endl;
          vector< vector<const Word*> > outputHope = decoder->getNBest(input, *sid, hope_n, 1.0, bleuWeight_hope,
              featureValuesHope[batchPosition], bleuScoresHope[batchPosition], modelScoresHope[batchPosition],
              1, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
          vector<const Word*> oracle = outputHope[0];
          decoder->cleanup(chartDecoding);
          ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
          avg_ref_length = ref_length;
          float hope_length_ratio = (float)oracle.size()/ref_length;
          cerr << endl;

          // count sparse features occurring in hope translation
          featureValuesHope[batchPosition][0].IncrementSparseHopeFeatures();

          vector<const Word*> bestModel;
          if (debug_model || historyBleu || simpleHistoryBleu) {
            // MODEL (for updating the history only, using dummy vectors)
            cerr << "Rank " << rank << ", epoch " << epoch << ", 1best wrt model score (debug or history)" << endl;
            vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, n, 0.0, bleuWeight,
                featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
                1, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
            bestModel = outputModel[0];
            decoder->cleanup(chartDecoding);
            cerr << endl;
            ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
          }

          // FEAR
          //float fear_length_ratio = 0;
          float bleuRatioHopeFear = 0;
          //int fearSize = 0;
          cerr << "Rank " << rank << ", epoch " << epoch << ", " << fear_n << "best fear translations" << endl;
          vector< vector<const Word*> > outputFear = decoder->getNBest(input, *sid, fear_n, -1.0, bleuWeight_fear,
              featureValuesFear[batchPosition], bleuScoresFear[batchPosition], modelScoresFear[batchPosition],
              1, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
          vector<const Word*> fear = outputFear[0];
          decoder->cleanup(chartDecoding);
          ref_length = decoder->getClosestReferenceLength(*sid, fear.size());
          avg_ref_length += ref_length;
          avg_ref_length /= 2;
          //fear_length_ratio = (float)fear.size()/ref_length;
          //fearSize = (int)fear.size();
          cerr << endl;
          for (size_t i = 0; i < fear.size(); ++i)
            delete fear[i];

          // count sparse features occurring in fear translation
          featureValuesFear[batchPosition][0].IncrementSparseFearFeatures();

          // Bleu-related example selection
          bool skip = false;
          bleuRatioHopeFear = bleuScoresHope[batchPosition][0] / bleuScoresFear[batchPosition][0];
          if (minBleuRatio != -1 && bleuRatioHopeFear < minBleuRatio)
            skip = true;
          if(maxBleuRatio != -1 && bleuRatioHopeFear > maxBleuRatio)
            skip = true;

          // sanity check
          if (historyBleu || simpleHistoryBleu) {
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
              } else {
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
            if (historyBleu || simpleHistoryBleu || debug_model) {
              featureValues[batchPosition].clear();
              bleuScores[batchPosition].clear();
            }
          } else {
            examples_in_batch++;

            // needed for history
            if (historyBleu || simpleHistoryBleu)  {
              inputLengths.push_back(current_input_length);
              ref_ids.push_back(*sid);
              oneBests.push_back(bestModel);
            }
          }
        }
        if (model_hope_fear) {
          cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best hope translations" << endl;
          size_t oraclePos = featureValues[batchPosition].size();
          decoder->getNBest(input, *sid, n, 1.0, bleuWeight_hope,
                            featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
                            0, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
          //vector<const Word*> oracle = outputHope[0];
          // needed for history
          inputLengths.push_back(current_input_length);
          ref_ids.push_back(*sid);
          decoder->cleanup(chartDecoding);
          //ref_length = decoder->getClosestReferenceLength(*sid, oracle.size());
          //float hope_length_ratio = (float)oracle.size()/ref_length;
          cerr << endl;

          oracleFeatureValues.push_back(featureValues[batchPosition][oraclePos]);
          oracleBleuScores.push_back(bleuScores[batchPosition][oraclePos]);
          oracleModelScores.push_back(modelScores[batchPosition][oraclePos]);

          // MODEL
          cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best wrt model score" << endl;
          if (historyBleu || simpleHistoryBleu) {
            vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, n, 0.0,
                bleuWeight, featureValues[batchPosition], bleuScores[batchPosition],
                modelScores[batchPosition], 1, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
            vector<const Word*> bestModel = outputModel[0];
            oneBests.push_back(bestModel);
            inputLengths.push_back(current_input_length);
            ref_ids.push_back(*sid);
          } else {
            decoder->getNBest(input, *sid, n, 0.0, bleuWeight,
                              featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
                              0, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
          }
          decoder->cleanup(chartDecoding);
          //ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
          //float model_length_ratio = (float)bestModel.size()/ref_length;
          cerr << endl;

          // FEAR
          cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best fear translations" << endl;
          decoder->getNBest(input, *sid, n, -1.0, bleuWeight_fear,
                            featureValues[batchPosition], bleuScores[batchPosition], modelScores[batchPosition],
                            0, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
          decoder->cleanup(chartDecoding);
          //ref_length = decoder->getClosestReferenceLength(*sid, fear.size());
          //float fear_length_ratio = (float)fear.size()/ref_length;

          examples_in_batch++;
        }
        if (kbest) {
          // MODEL
          cerr << "Rank " << rank << ", epoch " << epoch << ", " << n << "best wrt model score" << endl;
          if (historyBleu || simpleHistoryBleu) {
            vector< vector<const Word*> > outputModel = decoder->getNBest(input, *sid, n, 0.0,
                bleuWeight, featureValues[batchPosition], bleuScores[batchPosition],
                modelScores[batchPosition], 1, realBleu, distinctNbest, avgRefLength,	rank, epoch, "");
            vector<const Word*> bestModel = outputModel[0];
            oneBests.push_back(bestModel);
            inputLengths.push_back(current_input_length);
            ref_ids.push_back(*sid);
          } else {
            decoder->getNBest(input, *sid, n, 0.0, bleuWeight,
                              featureValues[batchPosition], bleuScores[batchPosition],
                              modelScores[batchPosition], 0, realBleu, distinctNbest, avgRefLength, rank, epoch, "");
          }
          decoder->cleanup(chartDecoding);
          //ref_length = decoder->getClosestReferenceLength(*sid, bestModel.size());
          //float model_length_ratio = (float)bestModel.size()/ref_length;
          cerr << endl;

          examples_in_batch++;

          HypothesisQueue queueHope(hope_n);
          HypothesisQueue queueFear(fear_n);
          cerr << endl;
          if (most_violated || all_violated) {
            float bleuHope = -1000;
            float bleuFear = 1000;
            int indexHope = -1;
            int indexFear = -1;

            vector<float> bleuHopeList;
            vector<float> bleuFearList;
            vector<float> indexHopeList;
            vector<float> indexFearList;

            if (most_violated)
              cerr << "Rank " << rank << ", epoch " << epoch << ", pick pair with most violated constraint" << endl;
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
              } else if (bleuScores[batchPosition][i] > bleuHope) { // better than current best
                bleuHope = bleuScores[batchPosition][i];
                indexHope = i;
              }
            }

            float currentViolation = 0;
            for (size_t i=0; i<bleuScores[batchPosition].size(); ++i) {
              float bleuDiff = bleuHope - bleuScores[batchPosition][i];
              float modelDiff = modelScores[batchPosition][indexHope] - modelScores[batchPosition][i];
              if ((bleuDiff > epsilon) && (modelDiff < bleuDiff)) {
                float diff = bleuDiff - modelDiff;
                if (diff > epsilon) {
                  if (all_violated) {
                    cerr << ".. adding pair";
                    bleuHopeList.push_back(bleuHope);
                    bleuFearList.push_back(bleuScores[batchPosition][i]);
                    indexHopeList.push_back(indexHope);
                    indexFearList.push_back(i);
                  } else if (most_violated && diff > currentViolation) {
                    currentViolation = diff;
                    bleuFear = bleuScores[batchPosition][i];
                    indexFear = i;
                    cerr << "Rank " << rank << ", epoch " << epoch << ", current violation: " << currentViolation << " (" << modelDiff << " >= " << bleuDiff << ")" << endl;
                  }
                }
              }
            }

            if (most_violated) {
              if (currentViolation > 0) {
                cerr << "Rank " << rank << ", epoch " << epoch << ", adding pair with violation " << currentViolation << endl;
                cerr << "Rank " << rank << ", epoch " << epoch << ", hope: " << bleuHope << " (" << indexHope  << "), fear: " << bleuFear << " (" << indexFear << ")" << endl;
                bleuScoresHope[batchPosition].push_back(bleuHope);
                bleuScoresFear[batchPosition].push_back(bleuFear);
                featureValuesHope[batchPosition].push_back(featureValues[batchPosition][indexHope]);
                featureValuesFear[batchPosition].push_back(featureValues[batchPosition][indexFear]);
                float modelScoreHope = modelScores[batchPosition][indexHope];
                float modelScoreFear = modelScores[batchPosition][indexFear];
                if (most_violated_reg) {
                  // reduce model score difference by factor ~0.5
                  float reg = currentViolation/4;
                  modelScoreHope += abs(reg);
                  modelScoreFear -= abs(reg);
                  float newViolation = (bleuHope - bleuFear) - (modelScoreHope - modelScoreFear);
                  cerr << "Rank " << rank << ", epoch " << epoch << ", regularized violation: " << newViolation << endl;
                }
                modelScoresHope[batchPosition].push_back(modelScoreHope);
                modelScoresFear[batchPosition].push_back(modelScoreFear);

                featureValues[batchPosition][indexHope].IncrementSparseHopeFeatures();
                featureValues[batchPosition][indexFear].IncrementSparseFearFeatures();
              } else {
                cerr << "Rank " << rank << ", epoch " << epoch << ", no violated constraint found." << endl;
                skip_example = 1;
              }
            } else cerr << endl;
          }
          if (max_bleu_diff) {
            cerr << "Rank " << rank << ", epoch " << epoch << ", pick pair with max Bleu diff from list: " << bleuScores[batchPosition].size() << endl;
            for (size_t i=0; i<bleuScores[batchPosition].size(); ++i) {
              float hopeScore = bleuScores[batchPosition][i];
              if (modelPlusBleu) hopeScore += modelScores[batchPosition][i];
              BleuIndexPair hope(hopeScore, i);
              queueHope.Push(hope);

              float fearScore = -1*(bleuScores[batchPosition][i]);
              if (modelPlusBleu) fearScore += modelScores[batchPosition][i];
              BleuIndexPair fear(fearScore, i);
              queueFear.Push(fear);
            }
            skip_example = 0;
          }
          cerr << endl;

          vector<BleuIndexPair> hopeList, fearList;
          for (size_t i=0; i<hope_n && !queueHope.Empty(); ++i) hopeList.push_back(queueHope.Pop());
          for (size_t i=0; i<fear_n && !queueFear.Empty(); ++i) fearList.push_back(queueFear.Pop());
          for (size_t i=0; i<hopeList.size(); ++i) {
            //float bleuHope = hopeList[i].first;
            size_t indexHope = hopeList[i].second;
            float bleuHope = bleuScores[batchPosition][indexHope];
            for (size_t j=0; j<fearList.size(); ++j) {
              //float bleuFear = -1*(fearList[j].first);
              size_t indexFear = fearList[j].second;
              float bleuFear = bleuScores[batchPosition][indexFear];
              cerr << "Rank " << rank << ", epoch " << epoch << ", hope: " << bleuHope << " (" << indexHope  << "), fear: " << bleuFear << " (" << indexFear << ")" << endl;
              bleuScoresHope[batchPosition].push_back(bleuHope);
              bleuScoresFear[batchPosition].push_back(bleuFear);
              featureValuesHope[batchPosition].push_back(featureValues[batchPosition][indexHope]);
              featureValuesFear[batchPosition].push_back(featureValues[batchPosition][indexFear]);
              float modelScoreHope = modelScores[batchPosition][indexHope];
              float modelScoreFear = modelScores[batchPosition][indexFear];

              modelScoresHope[batchPosition].push_back(modelScoreHope);
              modelScoresFear[batchPosition].push_back(modelScoreFear);

              featureValues[batchPosition][indexHope].IncrementSparseHopeFeatures();
              featureValues[batchPosition][indexFear].IncrementSparseFearFeatures();
            }
          }
          if (!makePairs)
            cerr << "Rank " << rank << ", epoch " << epoch << "summing up hope and fear vectors, no pairs" << endl;
        }

        // next input sentence
        ++sid;
        ++actualBatchSize;
        ++shardPosition;
      } // end of batch loop

      if (examples_in_batch == 0 || (kbest && skip_example)) {
        cerr << "Rank " << rank << ", epoch " << epoch << ", batch is empty." << endl;
      } else {
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
        vector<FeatureFunction*>::const_iterator iter;
        const vector<FeatureFunction*> &featureFunctions2 = FeatureFunction::GetFeatureFunctions();
        for (iter = featureFunctions2.begin(); iter != featureFunctions2.end(); ++iter) {
          if ((*iter)->GetScoreProducerDescription() == "BleuScoreFeature") {
            mosesWeights.Assign(*iter, 0);
            break;
          }
        }

        // scale LM feature (to avoid rapid changes)
        if (scale_lm) {
          cerr << "scale lm" << endl;
          const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
          for (size_t i = 0; i < statefulFFs.size(); ++i) {
            const StatefulFeatureFunction *ff = statefulFFs[i];
            const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff);

            if (lm) {
              // scale down score
              if (model_hope_fear) {
                scaleFeatureScore(lm, scale_lm_factor, featureValues, rank, epoch);
              } else {
                scaleFeatureScore(lm, scale_lm_factor, featureValuesHope, rank, epoch);
                scaleFeatureScore(lm, scale_lm_factor, featureValuesFear, rank, epoch);
              }
            }
          }
        }

        // scale WP
        if (scale_wp) {
          // scale up weight
          WordPenaltyProducer &wp = WordPenaltyProducer::InstanceNonConst();

          // scale down score
          if (model_hope_fear) {
            scaleFeatureScore(&wp, scale_wp_factor, featureValues, rank, epoch);
          } else {
            scaleFeatureScore(&wp, scale_wp_factor, featureValuesHope, rank, epoch);
            scaleFeatureScore(&wp, scale_wp_factor, featureValuesFear, rank, epoch);
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

        // apply learning rates to feature vectors before optimization
        if (feature_confidence) {
          cerr << "Rank " << rank << ", epoch " << epoch << ", apply feature learning rates with decays " << decay_core << "/" << decay_sparse << ": " << featureLearningRates << endl;
          if (model_hope_fear) {
            applyPerFeatureLearningRates(featureValues, featureLearningRates, sparse_r0);
          } else {
            applyPerFeatureLearningRates(featureValuesHope, featureLearningRates, sparse_r0);
            applyPerFeatureLearningRates(featureValuesFear, featureLearningRates, sparse_r0);
          }
        } else {
          // apply fixed learning rates
          cerr << "Rank " << rank << ", epoch " << epoch << ", apply fixed learning rates, core: " << core_r0 << ", sparse: " << sparse_r0 << endl;
          if (core_r0 != 1.0 || sparse_r0 != 1.0) {
            if (model_hope_fear) {
              applyLearningRates(featureValues, core_r0, sparse_r0);
            } else {
              applyLearningRates(featureValuesHope, core_r0, sparse_r0);
              applyLearningRates(featureValuesFear, core_r0, sparse_r0);
            }
          }
        }

        // Run optimiser on batch:
        VERBOSE(1, "\nRank " << rank << ", epoch " << epoch << ", run optimiser:" << endl);
        size_t update_status = 1;
        ScoreComponentCollection weightUpdate;
        if (perceptron_update) {
          vector<vector<float> > dummy1;
          update_status = optimiser->updateWeightsHopeFear( weightUpdate, featureValuesHope,
                          featureValuesFear, dummy1, dummy1, dummy1, dummy1, learning_rate, rank, epoch);
        } else if (hope_fear) {
          if (bleuScoresHope[0][0] >= min_oracle_bleu) {
            if (hope_n == 1 && fear_n ==1 && batchSize == 1 && !hildreth) {
              update_status = ((MiraOptimiser*) optimiser)->updateWeightsAnalytically(weightUpdate,
                              featureValuesHope[0][0], featureValuesFear[0][0], bleuScoresHope[0][0],
                              bleuScoresFear[0][0], modelScoresHope[0][0], modelScoresFear[0][0], learning_rate, rank, epoch);
            } else
              update_status = optimiser->updateWeightsHopeFear(weightUpdate, featureValuesHope,
                              featureValuesFear, bleuScoresHope, bleuScoresFear, modelScoresHope,
                              modelScoresFear, learning_rate, rank, epoch);
          } else
            update_status = 1;
        } else if (kbest) {
          if (batchSize == 1 && featureValuesHope[0].size() == 1 && !hildreth) {
            cerr << "Rank " << rank << ", epoch " << epoch << ", model score hope: " << modelScoresHope[0][0] << endl;
            cerr << "Rank " << rank << ", epoch " << epoch << ", model score fear: " << modelScoresFear[0][0] << endl;
            update_status = ((MiraOptimiser*) optimiser)->updateWeightsAnalytically(
                              weightUpdate, featureValuesHope[0][0], featureValuesFear[0][0],
                              bleuScoresHope[0][0], bleuScoresFear[0][0], modelScoresHope[0][0],
                              modelScoresFear[0][0], learning_rate, rank, epoch);
          } else {
            cerr << "Rank " << rank << ", epoch " << epoch << ", model score hope: " << modelScoresHope[0][0] << endl;
            cerr << "Rank " << rank << ", epoch " << epoch << ", model score fear: " << modelScoresFear[0][0] << endl;
            update_status = optimiser->updateWeightsHopeFear(weightUpdate, featureValuesHope,
                            featureValuesFear, bleuScoresHope, bleuScoresFear, modelScoresHope,
                            modelScoresFear, learning_rate, rank, epoch);
          }
        } else {
          // model_hope_fear
          update_status = ((MiraOptimiser*) optimiser)->updateWeights(weightUpdate,
                          featureValues, losses, bleuScores, modelScores, oracleFeatureValues,
                          oracleBleuScores, oracleModelScores, learning_rate, rank, epoch);
        }

        // sumStillViolatedConstraints += update_status;

        if (update_status == 0) {	 // if weights were updated
          // apply weight update
          if (debug)
            cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << weightUpdate << endl;

          if (feature_confidence) {
            // update confidence counts based on weight update
            confidenceCounts.UpdateConfidenceCounts(weightUpdate, signed_counts);

            // update feature learning rates
            featureLearningRates.UpdateLearningRates(decay_core, decay_sparse, confidenceCounts, core_r0, sparse_r0);
          }

          // apply weight update to Moses weights
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
          //cerr << "Rank " << rank << ", epoch " << epoch << ", new weights: " << mosesWeights << endl;
        }

        // update history (for approximate document Bleu)
        if (historyBleu || simpleHistoryBleu) {
          for (size_t i = 0; i < oneBests.size(); ++i)
            cerr << "Rank " << rank << ", epoch " << epoch << ", update history with 1best length: " << oneBests[i].size() << " ";
          decoder->updateHistory(oneBests, inputLengths, ref_ids, rank, epoch);
          deleteTranslations(oneBests);
        }
      } // END TRANSLATE AND UPDATE BATCH

      // size of all shards except for the last one
      size_t generalShardSize;
      if (trainWithMultipleFolds)
        generalShardSize = order.size()/coresPerFold;
      else
        generalShardSize = order.size()/size;

      size_t mixing_base = mixingFrequency == 0 ? 0 : generalShardSize / mixingFrequency;
      size_t dumping_base = weightDumpFrequency == 0 ? 0 : generalShardSize / weightDumpFrequency;
      bool mix = evaluateModulo(shardPosition, mixing_base, actualBatchSize);

      // mix weights?
      if (mix) {
#ifdef MPI_ENABLE
        cerr << "Rank " << rank << ", epoch " << epoch << ", mixing weights.. " << endl;
        // collect all weights in mixedWeights and divide by number of processes
        mpi::reduce(world, mosesWeights, mixedWeights, SCCPlus(), 0);

        // mix confidence counts
        //mpi::reduce(world, confidenceCounts, mixedConfidenceCounts, SCCPlus(), 0);
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
          //mixedConfidenceCounts.DivideEquals(size);

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

          if (weightMixingThisEpoch == mixingFrequency || reg_on_every_mix) {
            if (l1_regularize) {
              size_t pruned;
              if (l1_reg_sparse)
                pruned = mixedWeights.SparseL1Regularize(l1_lambda);
              else
                pruned = mixedWeights.L1Regularize(l1_lambda);
              cerr << "Rank " << rank << ", epoch " << epoch << ", "
                   << "l1-reg. on mixedWeights with lambda=" << l1_lambda << ", pruned: " << pruned << endl;
            }
            if (l2_regularize) {
              if (l2_reg_sparse)
                mixedWeights.SparseL2Regularize(l2_lambda);
              else
                mixedWeights.L2Regularize(l2_lambda);
              cerr << "Rank " << rank << ", epoch " << epoch << ", "
                   << "l2-reg. on mixedWeights with lambda=" << l2_lambda << endl;
            }
          }
        }

        // broadcast average weights from process 0
        mpi::broadcast(world, mixedWeights, 0);
        decoder->setWeights(mixedWeights);
        mosesWeights = mixedWeights;

        // broadcast summed confidence counts
        //mpi::broadcast(world, mixedConfidenceCounts, 0);
        //confidenceCounts = mixedConfidenceCounts;
#endif
#ifndef MPI_ENABLE
        //cerr << "\nRank " << rank << ", no mixing, weights: " << mosesWeights << endl;
        mixedWeights = mosesWeights;
#endif
      } // end mixing

      // Dump weights?
      if (trainWithMultipleFolds || weightEpochDump == weightDumpFrequency) {
        // dump mixed weights at end of every epoch to enable continuing a crashed experiment
        // (for jackknife every time the weights are mixed)
        ostringstream filename;
        if (epoch < 10)
          filename << weightDumpStem << "_mixed_0" << epoch;
        else
          filename << weightDumpStem << "_mixed_" << epoch;

        if (weightDumpFrequency > 1)
          filename << "_" << weightEpochDump;

        mixedWeights.Save(filename.str());
        cerr << "Dumping mixed weights during epoch " << epoch << " to " << filename.str() << endl << endl;
      }
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
      } else {
        if (evaluateModulo(shardPosition, dumping_base, actualBatchSize)) {
          cerr << "Rank " << rank << ", epoch " << epoch << ", dump weights.. (pos: " << shardPosition << ", base: " << dumping_base << ")" << endl;
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
            //FIXME: What do to for non-mpi version
            ScoreComponentCollection totalBinary;
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

              /*if (accumulateWeights) {
              cerr << "\nMixed average weights (cumulative) during epoch "	<< epoch << ": " << mixedAverageWeights << endl;
              } else {
              cerr << "\nMixed average weights during epoch " << epoch << ": " << mixedAverageWeights << endl;
              }*/

              cerr << "Dumping mixed average weights during epoch " << epoch << " to " << filename.str() << endl << endl;
              mixedAverageWeights.Save(filename.str());
              ++weightEpochDump;

              if (weightEpochDump == weightDumpFrequency) {
                if (l1_regularize) {
                  size_t pruned = mixedAverageWeights.SparseL1Regularize(l1_lambda);
                  cerr << "Rank " << rank << ", epoch " << epoch << ", "
                       << "l1-reg. on mixedAverageWeights with lambda=" << l1_lambda << ", pruned: " << pruned << endl;

                }
                if (l2_regularize) {
                  mixedAverageWeights.SparseL2Regularize(l2_lambda);
                  cerr << "Rank " << rank << ", epoch " << epoch << ", "
                       << "l2-reg. on mixedAverageWeights with lambda=" << l2_lambda << endl;
                }

                if (l1_regularize || l2_regularize) {
                  filename << "_reg";
                  cerr << "Dumping regularized mixed average weights during epoch " << epoch << " to " << filename.str() << endl << endl;
                  mixedAverageWeights.Save(filename.str());
                }
              }

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
      } // end if dump
    } // end of shard loop, end of this epoch
    cerr << "Rank " << rank << ", epoch " << epoch << ", end of epoch.." << endl;

    if (historyBleu || simpleHistoryBleu) {
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
          } else {
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

  if (rank == 0) {
    ScoreComponentCollection dummy;
    ostringstream endfilename;
    endfilename << "finished";
    dummy.Save(endfilename.str());
  }

  delete decoder;
  exit(0);
}

bool loadSentences(const string& filename, vector<string>& sentences)
{
  ifstream in(filename.c_str());
  if (!in)
    return false;
  string line;
  while (getline(in, line))
    sentences.push_back(line);
  return true;
}

bool evaluateModulo(size_t shard_position, size_t mix_or_dump_base, size_t actual_batch_size)
{
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
  } else {
    return ((shard_position % mix_or_dump_base) == 0);
  }
}

void printFeatureValues(vector<vector<ScoreComponentCollection> > &featureValues)
{
  for (size_t i = 0; i < featureValues.size(); ++i) {
    for (size_t j = 0; j < featureValues[i].size(); ++j) {
      cerr << featureValues[i][j] << endl;
    }
  }
  cerr << endl;
}

void deleteTranslations(vector<vector<const Word*> > &translations)
{
  for (size_t i = 0; i < translations.size(); ++i) {
    for (size_t j = 0; j < translations[i].size(); ++j) {
      delete translations[i][j];
    }
  }
}

void decodeHopeOrFear(size_t rank, size_t size, size_t decode, string filename, vector<string> &inputSentences, MosesDecoder* decoder, size_t n, float bleuWeight)
{
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
    bool realBleu = false;
    vector< vector<const Word*> > nbestOutput = decoder->getNBest(input, sid, n, factor, bleuWeight, dummyFeatureValues[0],
        dummyBleuScores[0], dummyModelScores[0], n, realBleu, true, false, rank, 0, "");
    cerr << endl;
    decoder->cleanup(StaticData::Instance().IsChart());

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

void applyLearningRates(vector<vector<ScoreComponentCollection> > &featureValues, float core_r0, float sparse_r0)
{
  for (size_t i=0; i<featureValues.size(); ++i) // each item in batch
    for (size_t j=0; j<featureValues[i].size(); ++j) // each item in nbest
      featureValues[i][j].MultiplyEquals(core_r0, sparse_r0);
}

void applyPerFeatureLearningRates(vector<vector<ScoreComponentCollection> > &featureValues, ScoreComponentCollection featureLearningRates, float sparse_r0)
{
  for (size_t i=0; i<featureValues.size(); ++i) // each item in batch
    for (size_t j=0; j<featureValues[i].size(); ++j) // each item in nbest
      featureValues[i][j].MultiplyEqualsBackoff(featureLearningRates, sparse_r0);
}

void scaleFeatureScore(const FeatureFunction *sp, float scaling_factor, vector<vector<ScoreComponentCollection> > &featureValues, size_t rank, size_t epoch)
{
  string name = sp->GetScoreProducerDescription();

  // scale down score
  float featureScore;
  for (size_t i=0; i<featureValues.size(); ++i) { // each item in batch
    for (size_t j=0; j<featureValues[i].size(); ++j) { // each item in nbest
      featureScore = featureValues[i][j].GetScoreForProducer(sp);
      featureValues[i][j].Assign(sp, featureScore*scaling_factor);
      //cerr << "Rank " << rank << ", epoch " << epoch << ", " << name << " score scaled from " << featureScore << " to " << featureScore/scaling_factor << endl;
    }
  }
}

void scaleFeatureScores(const FeatureFunction *sp, float scaling_factor, vector<vector<ScoreComponentCollection> > &featureValues, size_t rank, size_t epoch)
{
  string name = sp->GetScoreProducerDescription();

  // scale down score
  for (size_t i=0; i<featureValues.size(); ++i) { // each item in batch
    for (size_t j=0; j<featureValues[i].size(); ++j) { // each item in nbest
      vector<float> featureScores = featureValues[i][j].GetScoresForProducer(sp);
      for (size_t k=0; k<featureScores.size(); ++k)
        featureScores[k] *= scaling_factor;
      featureValues[i][j].Assign(sp, featureScores);
      //cerr << "Rank " << rank << ", epoch " << epoch << ", " << name << " score scaled from " << featureScore << " to " << featureScore/scaling_factor << endl;
    }
  }
}
