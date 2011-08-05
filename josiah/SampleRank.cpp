/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2009 University of Edinburgh
 
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
#include <functional>
#include <iostream>
#include <iomanip>
#include <fstream>

#ifdef MPI_ENABLED
#include <mpi.h>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
namespace mpi=boost::mpi;
#endif

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "Bleu.h"
#include "Decoder.h"
#include "GibbsOperator.h"
#include "Gibbler.h"
#include "InputSource.h"
#include "MpiDebug.h"
#include "OnlineLearner.h"
#include "OnlineTrainingCorpus.h"
#include "PhraseFeature.h"
#include "Sampler.h"
#include "SampleRankSelector.h"
#include "Utils.h"


using namespace std;
using namespace Josiah;
using namespace Moses;
using boost::is_any_of;
namespace po = boost::program_options;

static void MixWeights(size_t size, size_t rank) {
#ifdef MPI_ENABLED
    FVector avgWeights;
    FVector& currWeights = WeightManager::instance().get();
    MPI_VERBOSE(1, "Before mixing, current weights " << currWeights << endl);
    mpi::communicator world;
    mpi::reduce(world,currWeights,avgWeights, FVectorPlus(),0);
    if (rank == 0) {
      avgWeights /= size;
    }
    mpi::broadcast(world,avgWeights,0);
    WeightManager::instance().get() = avgWeights;
    MPI_VERBOSE(1, "After mixing, current weights: " << avgWeights << endl);
#endif
}


int main(int argc, char** argv) {
  int rank = 0, size = 1;
#ifdef MPI_ENABLED
  MPI_Init(&argc,&argv);
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Comm_rank(comm,&rank);
  MPI_Comm_size(comm,&size);
  cerr << "MPI rank: " << rank << endl; 
  cerr << "MPI size: " << size << endl;
#endif
  size_t iterations;
  string feature_file;
  int debug;
  int mpidebug;
  string mpidebugfile;
  int burning_its;
  string inputfile;
  string mosesini;
  bool help;
  uint32_t seed;
  string weightfile;
  vector<string> ref_files;
  size_t batchLines;
  size_t epochLines;
  size_t epochs;
  string weight_dump_stem;
  size_t weight_dump_batches;
  size_t weight_dump_samples;
  bool weight_dump_current;
  size_t lag;
  string learnerName;
  bool chiang_target;
  bool always_update;
  bool update_target;
  float cwInitialVariance, cwConfidence;
  float perceptron_lr;
  float fixed_temperature;
  float fixed_temperature_scaling;
  bool slack_rescaling, scale_loss_by_target_gain;
  vector<float> burnin_anneal;
  bool closestBestNeighbour;
  bool approxDocBleu;
  float approxDocBleuDecay;
  bool fix_margin;
  float margin, slack;
  float tolerance;
  bool ignoreUWP;
  bool disableUWP;
  bool l1Normalise, l2Normalise;
  float norm, scale_margin;
  float flip_prob, merge_split_prob, retrans_prob;
  size_t merge_split_toptions, retrans_toptions;
  bool enable_trans_options_cache;
  bool use_alignment_info;
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
  ("config,f",po::value<string>(&mosesini),"Moses ini file")
  ("verbosity,v", po::value<int>(&debug)->default_value(0), "Verbosity level")
  ("mpi-debug-level", po::value<int>(&MpiDebug::verbosity)->default_value(0), "Verbosity level for debugging messages used in mpi.")
  ("mpi-debug-file", po::value<string>(&mpidebugfile), "Debug file stem for use by mpi processes")
  ("random-seed,e", po::value<uint32_t>(&seed), "Random seed")
  ("iterations,s", po::value<size_t>(&iterations)->default_value(10), 
   "Number of sampler iterations")
  ("burn-in,b", po::value<int>(&burning_its)->default_value(1), "Duration (in sampling iterations) of burn-in period")
  ("input-file,i",po::value<string>(&inputfile),"Input file containing tokenised source")
  ("weights,w",po::value<string>(&weightfile),"Weight file")
  ("ref,r", po::value<vector<string> >(&ref_files), "Reference translation files for training")
  ("extra-feature-config,X", po::value<string>(&feature_file), "Configuration file for extra (non-Moses) features")
  ("batch-lines", po::value<size_t>(&batchLines)->default_value(1), "Number of lines in each training batch")
  ("epoch-lines", po::value<size_t>(&epochLines)->default_value(1000), "Number of lines in each epoch")
  ("epochs", po::value<size_t>(&epochs)->default_value(1), "Number of training epochs")
  ("weight-dump-stem", po::value<string>(&weight_dump_stem)->default_value(""), "Stem of filename to use for dumping weights - leave empty for no dumping")
  ("weight-dump-batches", po::value<size_t>(&weight_dump_batches)->default_value(0), "Number of batches to process before dumping weights")
  ("weight-dump-samples", po::value<size_t>(&weight_dump_samples)->default_value(0), "Number of samples to process before dumping weights")
  ("weight-dump-current",po::value<bool>(&weight_dump_current)->zero_tokens()->default_value(false), "Dump the current weights, instead of the averaged weights")
  ("lag", po::value<size_t>(&lag)->default_value(1), "How often  to collect weight updates for the average weights.")
  ("learner", po::value(&learnerName)->default_value("perceptron"), "Use this online learner")
  ("always-update", po::value<bool>(&always_update)->zero_tokens()->default_value(false),
    "Always call the update, even if ranking is correct")
  ("update-target", po::value<bool>(&update_target)->zero_tokens()->default_value(false),
    "Update towards target, not chosen")
  ("chiang-target", po::value<bool>(&chiang_target)->zero_tokens()->default_value(false),
    "Use Chiang's gain+score to choose the target")
  ("cw-initial-variance", po::value<float>(&cwInitialVariance)->default_value(1.0f), "Initial variance for CW Learning")
  ("cw-confidence", po::value<float>(&cwConfidence)->default_value(1.644854f), "Initial confidence value for CW Learning, use value in probit([0.5,1.0])")
  ("perc-lr", po::value<float>(&perceptron_lr)->default_value(1.0f), "Perceptron learning rate")
  ("use-slack-rescaling", po::value<bool>(&slack_rescaling)->zero_tokens()->default_value(false), "Use slack rescaling in mira (default is margin rescaling)")
  ("scale-loss-by-target-gain", po::value<bool>(&scale_loss_by_target_gain)->zero_tokens()->default_value(false), "Scale the loss by the target gain")
  ("fixed-temperature", po::value<float>(&fixed_temperature)->default_value(1.0f), "Temperature for fixed temp sample acceptor")
  ("scale-fixed-temperature", po::value<float>(&fixed_temperature_scaling)->default_value(1.0f), "Scaling applied to fixed temperature at the end of an epoch")
  ("burnin-anneal", po::value<vector<float> >(&burnin_anneal)->multitoken(), "Specify (start stop floor ratio) for burnin annealing")
  ("closest-best-neighbour", po::value(&closestBestNeighbour)->zero_tokens()->default_value(false), "Closest best neighbour")
  ("use-approx-doc-bleu", po::value(&approxDocBleu)->zero_tokens()->default_value(false), "Compute approx doc bleu as gain")
  ("approx-doc-bleu-decay", po::value<float>(&approxDocBleuDecay)->default_value(0.9), "Decay factor for approx doc bleu")
  ("fix-margin", po::value(&fix_margin)->zero_tokens()->default_value(false), "Do MIRA update with a specified margin")
  ("margin", po::value<float>(&margin)->default_value(1.0f), "Margin size")
  ("slack", po::value<float>(&slack)->default_value(-1.0f), "Slack")
  ("tolerance", po::value<float>(&tolerance)->default_value(0.0f), "Difference between chosen bleu and target bleu must be greater than this to force a weight update")
  ("ignore-uwp", po::value<bool>(&ignoreUWP)->zero_tokens()->default_value(false), "Ignore unknown word penalty weight when training")
  ("disable-uwp", po::value<bool>(&disableUWP)->zero_tokens()->default_value(false), "Disable the unknown word penalty weight when training")
  ("l1normalise", po::value<bool>(&l1Normalise)->zero_tokens()->default_value(false), "L1normalise weight vector during MIRA samplerank training")
  ("l2normalise", po::value<bool>(&l2Normalise)->zero_tokens()->default_value(false), "L2normalise weight vector during MIRA samplerank training")
  ("norm", po::value<float>(&norm)->default_value(1.0f), "Normalise weight vector to this value")
  ("margin-scale", po::value<float>(&scale_margin)->default_value(1.0f), "Scale margin by this factor")
  ("enable-trans-options-cache", po::value<bool>(&enable_trans_options_cache)->zero_tokens()->default_value(false), "Enable the translation options cache")
  ("flip-prob", po::value<float>(&flip_prob)->default_value(0.6f), "Probability of applying flip operator during random scan")
  ("merge-split-prob", po::value<float>(&merge_split_prob)->default_value(0.2f), "Probability of applying merge-split operator during random scan")
  ("retrans-prob", po::value<float>(&retrans_prob)->default_value(0.2f), "Probability of applying retrans operator during random scan")
  ("merge-split-toptions", po::value<size_t>(&merge_split_toptions)->default_value(20), "Maximum number of translation options for merge-split")
  ("retrans-toptions", po::value<size_t>(&retrans_toptions)->default_value(20), "Maximum number of translation options for retrans")
  ("use-alignment-info",po::value<bool>(&use_alignment_info)->zero_tokens()->default_value(false), "Load the alignment info from the phrase table")
  ;
 
  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);
  
  
  
  if (help) {
    std::cout << "Usage: " + string(argv[0]) +  " -f mosesini-file [options]" << std::endl;
    std::cout << desc << std::endl;
    return 0;
  }
  
  if (weightfile.empty()) {
    std::cerr << "Setting all feature weights to zero" << std::endl;
    WeightManager::init();
  } else {
    std::cerr << "Loading feature weights from " << weightfile <<  std::endl;
    WeightManager::init(weightfile);
  }
  
  if (mosesini.empty()) {
    cerr << "Error: No moses ini file specified" << endl;
    return 1;
  }
  
  if (mpidebugfile.length()) {
    MpiDebug::init(mpidebugfile,rank);
  }
   
  float opProb = flip_prob + merge_split_prob + retrans_prob;
  if (fabs(1.0 - opProb) > 0.00001) {
    std::cerr << "Incorrect usage: specified operator probs should sum up to 1" << std::endl;
    return 0;  
  }

  if (burnin_anneal.size() && burnin_anneal.size() != 4) {
    cerr << "Error: --burnin-anneal requires 4 arguments" << endl;
    return 1;
  }

  if (weight_dump_stem.size() && !weight_dump_samples && !weight_dump_batches) {
    cerr << "Error: If weight_dump_stem is set then must specify either " << endl;
    cerr << "       --weight_dump_samples or --weight_dump_batches" << endl;
    return 1;
  }

  if (weight_dump_samples && weight_dump_batches) {
    cerr << "Error: Must specify either --weight-dump-samples or --weight-dump-batches" << endl;
    return 1;
  }
  
  //set up moses
  vector<string> extraArgs;
  extraArgs.push_back("-ttable-limit");
  size_t ttableLimit = max(merge_split_toptions, retrans_toptions);
  ostringstream ttableLimitConfig;
  ttableLimitConfig << ttableLimit;
  extraArgs.push_back(ttableLimitConfig.str());
  if (!enable_trans_options_cache) {
    extraArgs.push_back("-persistent-cache-size");
    extraArgs.push_back("0");
  }
  if (use_alignment_info) {
    extraArgs.push_back("-use-alignment-info");
  }
  initMoses(mosesini,debug,extraArgs);
  
  FeatureVector features;
  FVector coreWeights;
  configure_features_from_file(feature_file, features,disableUWP,coreWeights);
  std::cerr << "Using " << features.size() << " features" << std::endl;
  
  
  if (vm.count("random-seed")) {
    RandomNumberGenerator::instance().setSeed(seed + rank);
  }      

  auto_ptr<Bleu> bleu(new Bleu());
  if (approxDocBleu) {
    bleu->SetSmoothingWeight(approxDocBleuDecay);
  }
  auto_ptr<Gain> gain(bleu);
  gain->LoadReferences(ref_files,inputfile);
  
  Sampler sampler;
  sampler.SetLag(1); //thinning factor for sample collection

  //configure the sampler
  MergeSplitOperator mso(merge_split_prob,merge_split_toptions);
  FlipOperator fo(flip_prob);
  TranslationSwapOperator tso(retrans_prob,retrans_toptions);
    
  sampler.AddOperator(&mso);
  sampler.AddOperator(&tso);
  sampler.AddOperator(&fo);
  
  //Target Assigner
  TargetAssignerHandle tgtAssigner;
  if (closestBestNeighbour) {
    tgtAssigner.reset(new ClosestBestNeighbourTgtAssigner());
  } else if (chiang_target) {
    tgtAssigner.reset(new ChiangBestNeighbourTgtAssigner());
  }
  else {
    tgtAssigner.reset(new BestNeighbourTgtAssigner());
  }
  
  
  
  
  //Add the learner
  OnlineLearnerHandle onlineLearner;
  
  if (learnerName == "perceptron") {
    boost::shared_ptr<PerceptronLearner> perceptron(new PerceptronLearner());
    perceptron->setLearningRate(perceptron_lr);
    onlineLearner = perceptron;
  } else if (learnerName == "mira+") {
    boost::shared_ptr<MiraPlusLearner> mp(new MiraPlusLearner());
    mp->setSlack(slack);
    mp->setMarginScale(scale_margin);
    mp->setFixMargin(fix_margin);
    mp->setMargin(margin);
    onlineLearner = mp;
  } else if (learnerName == "mira") {
    boost::shared_ptr<MiraLearner> m(new MiraLearner());
    m->setSlack(slack);
    m->setMarginScale(scale_margin);
    m->setFixMargin(fix_margin);
    m->setMargin(margin);
    m->setUseSlackRescaling(slack_rescaling);
    m->setScaleLossByTargetGain(scale_loss_by_target_gain);
    onlineLearner = m;
  } else {
    throw runtime_error("Unknown learner: " + learnerName);
  }

  sampler.SetIterations(iterations);
  sampler.SetBurnIn(burning_its);
  
  OnlineTrainingCorpus trainingCorpus
      (inputfile,
       batchLines,
       epochLines,
       epochs*epochLines,
       size,
       rank);
  
  bool byBatch  = false;
  size_t weightDumpFrequency = weight_dump_samples;
  if (!weightDumpFrequency) {
    weightDumpFrequency = weight_dump_batches;
    byBatch = true;
  }
  WeightCollectorHandle weightCollector(
    new WeightCollector(weightDumpFrequency,byBatch,weight_dump_stem,size,rank));
  weightCollector->SetL1Normalise(l1Normalise);
  weightCollector->SetDumpCurrent(weight_dump_current);
  weightCollector->SetLag(lag);
  
  while (trainingCorpus.HasMore()) {
    vector<string> lines;
    vector<size_t> lineNumbers;
    bool shouldMix;
    trainingCorpus.GetNextBatch(&lines,&lineNumbers, &shouldMix);
     
    //Makes sure that t-options get sorted by the appropriate weights
    FVector currentWeights;
    if (coreWeights != FVector()) {
      currentWeights = coreWeights;
    } else if (weightCollector->getBatchCount()) {
      currentWeights = weightCollector->getAverageWeights();
    } else {
      currentWeights = WeightManager::instance().get();
    }
    setMosesWeights(currentWeights);

    
    //Generate random hypotheses
    vector<TranslationHypothesis> translations;
    for (size_t i = 0; i < lines.size(); ++i) {
      translations.push_back(TranslationHypothesis(lines[i]));
      cerr << "Source sentence: " << lines[i] << endl;
      cerr << "Seed hypothesis: " << *(translations.back().getHypothesis()) << endl;
    }
    
    
    
    //The selector for this sentence
    GainFunctionHandle gf = gain->GetGainFunction(lineNumbers);
    SampleRankSelector selector(gf, onlineLearner, tgtAssigner, weightCollector);
    selector.SetTemperature(fixed_temperature);
    //burnin annealing
    auto_ptr<AnnealingSchedule> annealer;
    if (burnin_anneal.size()) {
      annealer.reset(new ExponentialAnnealingSchedule
        (burnin_anneal[0],burnin_anneal[1], burnin_anneal[2], burnin_anneal[3]));
    } else {
      //fixed temp
      annealer.reset(new ExponentialAnnealingSchedule
      (fixed_temperature,fixed_temperature,fixed_temperature,1));
    }
    selector.SetBurninAnnealer(annealer.get());
    selector.SetIgnoreUnknownWordPenalty(ignoreUWP);
    selector.SetTolerance(tolerance);
    selector.SetAlwaysUpdate(always_update);
    selector.SetUpdateTarget(update_target);
    sampler.SetSelector(&selector);
    
    
    
    sampler.Run(translations,features);
    //cerr << "Performed " << onlineLearner->GetNumUpdates() << " updates for this sentence" << endl;


    if (size == 1) {
      cerr <<  "Batch count: " << weightCollector->getBatchCount() << endl;
      cerr << "Curr Weights : " << WeightManager::instance().get() << endl;
      cerr << "Average Weights : " << weightCollector->getAverageWeights() << endl;
    } else {
      MPI_VERBOSE(1,"Batch count: " << weightCollector->getBatchCount() << endl);
      MPI_VERBOSE(1,"Current Weights : " << WeightManager::instance().get() << endl);
      MPI_VERBOSE(1,"Average Weights : " << weightCollector->getAverageWeights() << endl);
    }
    
    //PhraseFeature::updateWeights(WeightManager::instance().get());
    if (approxDocBleu) {
      //This sends the smoothing stats from gf to gain, and resets gf's smoothing stats
      gf->UpdateSmoothingStats();
    }
    if (shouldMix) {
      MixWeights(size,rank);
      fixed_temperature *= fixed_temperature_scaling;
      VERBOSE(1,"Fixed temperature scaled by " << fixed_temperature_scaling << " to " << fixed_temperature << endl);
    }
    weightCollector->endBatch();
  } 
  
#ifdef MPI_ENABLED
  MPI_Finalize();
#endif
  return 0;
}
