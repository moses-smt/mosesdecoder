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
#endif

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "AnnealingSchedule.h"
#include "Bleu.h"
#include "Decoder.h"
#include "Derivation.h"
#include "FeatureVector.h"
#include "Gibbler.h"
#include "InputSource.h"
#include "TrainingSource.h"
#include "GibbsOperator.h"
#include "Gain.h"
#include "GibblerExpectedLossTraining.h"
#include "GibblerAnnealedExpectedLossTrainer.h"
#include "GibblerMaxTransDecoder.h"
#include "MpiDebug.h"
#include "Selector.h"
#include "StaticData.h"
#include "Optimizer.h"
#include "Utils.h"
#include "WeightManager.h"


using namespace std;
using namespace Josiah;
using namespace Moses;
using boost::lexical_cast;
using boost::bad_lexical_cast;
using boost::split;
using boost::is_any_of;
namespace po = boost::program_options;


/**
  * Main for Josiah - the Gibbs sampler for moses.
 **/
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
  GibbsTimer timer;
  size_t iterations;
  unsigned int topn;
  int debug;
  int mpidebug;
  string mpidebugfile;
  string feature_file;
  int burning_its;
  int mbr_size, topNsize;
  string inputfile;
  string outputfile;
  string mosesini;
  bool decode;
  bool translate;
  bool translation_distro;
  bool derivation_distro;
  bool help;
  bool expected_sbleu;
  bool expected_sbleu_da;
  bool output_expected_sbleu;
  unsigned training_batch_size;
  bool mbr_decoding;
  bool do_timing;
  int max_training_iterations;
  uint32_t seed;
  int lineno;
  bool randomize;
  FValue scalefactor;
  FValue eta;
  FValue mu;
  string weightfile;
  vector<string> ref_files;
  int periodic_decode;
  bool collect_dbyt;
  bool output_max_change;
  bool anneal;
  unsigned int reheatings;
  float max_temp;
  FValue prior_variance;
  FValue prior_mean;
  string prev_gradient_file;
  float start_temp_expda;
  float stop_temp_expda;
  float floor_temp_expda; 
  float anneal_ratio_da;
  float gamma;
  bool use_metanormalized_egd;
  int optimizerFreq; 
  int weight_dump_freq;
  string weight_dump_stem;
  int init_iteration_number;
  bool greedy, fixedTemp;
  float fixed_temperature;
  bool mapdecode;
  vector<string> ngramorders;
  bool raoBlackwell;
  bool use_moses_kbesthyposet;
  bool print_moseskbest;
  bool randomScan;
  size_t lag;
  float flip_prob, merge_split_prob, retrans_prob;
  bool calc_exact_posterior, filter_by_posterior;
  float evidenceSetShrinkFactor;
  bool randomShrink;
  float log_base_factor;
  bool checkFeatures;
  po::options_description desc("Allowed options");
  desc.add_options()
        ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
        ("config,f",po::value<string>(&mosesini),"Moses ini file")
        ("verbosity,v", po::value<int>(&debug)->default_value(0), "Verbosity level")
        ("mpi-debug-level", po::value<int>(&MpiDebug::verbosity)->default_value(0), "Verbosity level for debugging messages used in mpi.")
        ("mpi-debug-file", po::value<string>(&mpidebugfile), "Debug file stem for use by mpi processes")
        ("random-seed,e", po::value<uint32_t>(&seed), "Random seed")
        ("timing,m", po::value(&do_timing)->zero_tokens()->default_value(false), "Display timing information.")
      ("iterations,s", po::value<size_t>(&iterations)->default_value(10), 
       "Number of sampler iterations")
        ("burn-in,b", po::value<int>(&burning_its)->default_value(1), "Duration (in sampling iterations) of burn-in period")
        ("scale-factor,c", po::value<FValue>(&scalefactor)->default_value(1.0), "Scale factor for model weights.")
        ("input-file,i",po::value<string>(&inputfile),"Input file containing tokenised source")
        ("output-file-prefix,o",po::value<string>(&outputfile),"Output file prefix for translations, MBR output, etc")
        ("nbest-drv,n",po::value<unsigned int>(&topn)->default_value(0),"Write the top n derivations to stdout")
	("weights,w",po::value<string>(&weightfile),"Weight file")
        ("decode-derivation,d",po::value( &decode)->zero_tokens()->default_value(false),"Write the most likely derivation to stdout")
        ("decode-translation,t",po::value(&translate)->zero_tokens()->default_value(false),"Write the most likely translation to stdout")
      ("distro-derivation", po::value(&derivation_distro)->zero_tokens()->default_value(false), "Print derivation probability distribution")
      ("distro-translation", po::value(&translation_distro)->zero_tokens()->default_value(false), "Print translation probability distribution")
        ("periodic-derivation,p",po::value(&periodic_decode)->default_value(0), "Periodically write the max derivation to stderr")
      ("max-change", po::value(&output_max_change)->zero_tokens()->default_value(false), "Whenever the max deriv or max trans changes, write it to stderr")
        ("collect-dbyt",po::value(&collect_dbyt)->zero_tokens()->default_value(false), "Collect derivations per translation")
        ("line-number,L", po::value(&lineno)->default_value(0), "Starting reference/line number")
        ("randomize-batches,R", po::value(&randomize)->zero_tokens()->default_value(false), "Randomize training batches")
        ("gaussian-prior-variance", po::value<FValue>(&prior_variance)->default_value(0.0f), "Gaussian prior variance (0 for no prior)")
        ("gaussian-prior-mean,P", po::value<FValue>(&prior_mean)->default_value(0.0f), "Gaussian prior mean")
        ("expected-bleu-training,T", po::value(&expected_sbleu)->zero_tokens()->default_value(false), "Train to maximize expected sentence BLEU")
          ("output-expected-sbleu", po::value(&output_expected_sbleu)->zero_tokens()->default_value(false), "Output expected bleu and feature expectations at end of sampling")
        ("max-training-iterations,M", po::value(&max_training_iterations)->default_value(30), "Maximum training iterations")
        ("training-batch-size,S", po::value(&training_batch_size)->default_value(0), "Batch size to use during xpected bleu training, 0 = full corpus")
	("reheatings", po::value<unsigned int>(&reheatings)->default_value(1), "Number of times to reheat the sampler")
	("anneal,a", po::value(&anneal)->default_value(false)->zero_tokens(), "Use annealing during the burn in period")
	("max-temp", po::value<float>(&max_temp)->default_value(4.0), "Annealing maximum temperature")
        ("eta", po::value<FValue>(&eta), "Default learning rate for SGD/EGD")
        ("prev-gradient", po::value<string>(&prev_gradient_file), "File containing previous gradient for restarting SGD/EGD")
        ("mu", po::value<float>(&mu)->default_value(1.0f), "Metalearning rate for EGD")
        ("gamma", po::value<float>(&gamma)->default_value(0.9f), "Smoothing parameter for Metanormalized EGD ")
        ("mbr-size", po::value<int>(&mbr_size)->default_value(200),"Number of samples to use for MBR decoding")
        ("mbr", po::value(&mbr_decoding)->zero_tokens()->default_value(false), "Minimum Bayes Risk Decoding")
        ("topn-size", po::value<int>(&topNsize)->default_value(0),"Number of samples to use for inner loop of MBR decoding")
        ("ref,r", po::value<vector<string> >(&ref_files), "Reference translation files for training")
      ("extra-feature-config,X", po::value<string>(&feature_file), "Configuration file for extra (non-Moses) features")
        ("check-features", po::value<bool>(&checkFeatures)->zero_tokens()->default_value(false), "Check features for consistency after every update")
        ("use-metanormalized-egd,N", po::value(&use_metanormalized_egd)->zero_tokens()->default_value(false), "Use metanormalized EGD")
        ("expected-bleu-deterministic-annealing-training,D", po::value(&expected_sbleu_da)->zero_tokens()->default_value(false), "Train to maximize expected sentence BLEU using deterministic annealing")   
        ("optimizer-freq", po::value<int>(&optimizerFreq)->default_value(1),"Number of optimization to perform at given temperature")
   ("initial-det-anneal-temp", po::value<float>(&start_temp_expda)->default_value(1000.0f), "Initial deterministic annealing entropy temperature")
   ("final-det-anneal-temp", po::value<float>(&stop_temp_expda)->default_value(0.001f), "Final deterministic annealing entropy temperature")
   ("floor-temp", po::value<float>(&floor_temp_expda)->default_value(0.0f), "Floor temperature for det annealing")
  ("det-annealing-ratio,A", po::value<float>(&anneal_ratio_da)->default_value(0.5f), "Deterministc annealing ratio")
      ("weight-dump-freq", po::value<int>(&weight_dump_freq)->default_value(0), "Frequency to dump weight files during training")
  ("weight-dump-stem", po::value<string>(&weight_dump_stem)->default_value("weights"), "Stem of filename to use for dumping weights")
      ("init-iteration-number", po::value<int>(&init_iteration_number)->default_value(0), "First training iteration will be one after this (useful for restarting)")
 ("greedy", po::value(&greedy)->zero_tokens()->default_value(false), "Greedy sample acceptor")
  ("fixed-temp-accept", po::value(&fixedTemp)->zero_tokens()->default_value(false), "Fixed temperature sample acceptor")
  ("fixed-temperature", po::value<float>(&fixed_temperature)->default_value(1.0f), "Temperature for fixed temp sample acceptor")
  ("rao-blackwell", po::value(&raoBlackwell)->zero_tokens()->default_value(false), "Do Rao-Blackwellisation (aka conditional estimation")
  ("mapdecode", po::value(&mapdecode)->zero_tokens()->default_value(false), "MAP decoding")
  ("mh.ngramorders", po::value< vector <string> >(&ngramorders), "Indicate LMs and ngram orders to be used during MH/Gibbs")
  ("use-moses-kbesthyposet", po::value(&use_moses_kbesthyposet)->zero_tokens()->default_value(false), "Use Moses to generate kbest hypothesis set")
  ("print-moseskbest", po::value(&print_moseskbest)->zero_tokens()->default_value(false), "Print Moses kbest")
  ("lag", po::value<size_t>(&lag)->default_value(10), "Lag between collecting samples")
  ("flip-prob", po::value<float>(&flip_prob)->default_value(0.6f), "Probability of applying flip operator during random scan")
  ("merge-split-prob", po::value<float>(&merge_split_prob)->default_value(0.2f), "Probability of applying merge-split operator during random scan")
  ("retrans-prob", po::value<float>(&retrans_prob)->default_value(0.2f), "Probability of applying retrans operator during random scan")
  ("calc-exact-post", po::value(&calc_exact_posterior)->zero_tokens()->default_value(false), "Calculate exact posterior")
  ("filter-exact-post", po::value(&filter_by_posterior)->zero_tokens()->default_value(false), "Filter sample set using exact posterior")
  ("evidence-shrink",  po::value<float>(&evidenceSetShrinkFactor)->default_value(0.9f), "Evidence set shrink factor for MBR decoding")
  ("random-shrink",  po::value(&randomShrink)->zero_tokens()->default_value(false), "Shrink evidence set randomly, otherwise shrink by discarding low probability elements")
          ("log-base-factor", po::value<float>(&log_base_factor)->default_value(1.0f), "Scaling factor for log probabilities in translation and language models");
 
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

  if (expected_sbleu && expected_sbleu_da) {
    std::cerr << "Incorrect usage: Cannot do both expected bleu training and expected bleu deterministic annealing training" << std::endl;
    return 0;
  }

  float opProb = flip_prob + merge_split_prob + retrans_prob;
  if (fabs(1.0 - opProb) > 0.00001) {
    std::cerr << "Incorrect usage: specified operator probs should sum up to 1" << std::endl;
    return 0;  
  }
  
  
  
  if (translation_distro) translate = true;
  if (derivation_distro) decode = true;
  
  
  if (mosesini.empty()) {
      cerr << "Error: No moses ini file specified" << endl;
      return 1;
  }

  if (mpidebugfile.length()) {
      MpiDebug::init(mpidebugfile,rank);
  }
  cerr << "optimizer freq " << optimizerFreq << endl;
  assert(optimizerFreq != 0);
  
  if (do_timing) {
    timer.on();
  }
  
  if (log_base_factor != 1.0) {
      //cerr << "Setting log base factor to " << log_base_factor << endl;
      cerr << "setting log base factor disabled" << endl;
      exit(1);
      //SetLogBaseFactor(log_base_factor);
  }
  
   //set up moses
  initMoses(mosesini,debug);
  
  FeatureVector features; 
  FVector coreWeights;
  configure_features_from_file(feature_file, features, false, coreWeights);
  std::cerr << "Using " << features.size() << " features" << std::endl;
  
  
  //scale model weights
  WeightManager::instance().scale(scalefactor);
  VERBOSE(1,"Scaled weights by factor of " << scalefactor << endl);
  
  

  if (vm.count("random-seed")) {
    RandomNumberGenerator::instance().setSeed(seed + rank);
  }      
      
  auto_ptr<Gain> gain;
  
  ostream* out = &cout;
  if (!outputfile.empty()) {
    ostringstream os;
    os << setfill('0');
    os << outputfile << '.' << setw(3) << rank << "_of_" << size;
    VERBOSE(1, "Writing output to: " << os.str() << endl);
    out = new ofstream(os.str().c_str());
  }
  auto_ptr<istream> in;
  auto_ptr<InputSource> input;
  
  auto_ptr<Optimizer> optimizer;
  FVector etaVector(eta);


  
  FVector prev_gradient;
  if (!prev_gradient_file.empty()) {
    prev_gradient.load(prev_gradient_file);
  }

  if (use_metanormalized_egd) {
    optimizer.reset(new MetaNormalizedExponentiatedGradientDescent(
                                                             etaVector,
                                                             mu,
                                                             0.1,   // minimal step scaling factor
                                                             gamma,                                       
                                                             max_training_iterations,
                                                             prev_gradient));
  } else {
    optimizer.reset(new ExponentiatedGradientDescent(
                                                                   etaVector,
                                                                   mu,
                                                                   0.1f,   // minimal step scaling factor
                                                                   max_training_iterations,
                                                                   prev_gradient));
  }
  if (optimizer.get()) {
      optimizer->SetIteration(init_iteration_number);
  }
  if (prior_variance != 0.0f) {
    assert(prior_variance > 0);
    std::cerr << "Using Gaussian prior: \\sigma^2=" << prior_variance << " \\mu=" << prior_mean <<  endl;
    optimizer->SetUseGaussianPrior(prior_mean, prior_variance);
  }
  ExpectedBleuTrainer* trainer = NULL;
  if (expected_sbleu || expected_sbleu_da) {
    gain.reset(new Bleu());
    gain->LoadReferences(ref_files,inputfile);
    vector<string> input_lines;
    ifstream infiles(inputfile.c_str());
    assert (infiles);
    while(infiles) {
      string line;
      getline(infiles, line);
      if (line.empty() && infiles.eof()) break;
      assert(!line.empty());
      input_lines.push_back(line);
    }
    VERBOSE(1, "Loaded " << input_lines.size() << " lines in training mode" << endl);
    if (!training_batch_size || training_batch_size > input_lines.size())
      training_batch_size = input_lines.size();
    VERBOSE(1, "Batch size: " << training_batch_size << endl);
    trainer = new ExpectedBleuTrainer(rank, size, training_batch_size, &input_lines, seed, randomize, optimizer.get(),     
                                      weight_dump_freq, weight_dump_stem);
    input.reset(trainer);
  } else {
    if (inputfile.size()) {
      input.reset(new BatchedFileInputSource(inputfile,rank,size));
    } else {
      input.reset(new StreamInputSource(cin));
    }
  }
  
  auto_ptr<SamplingSelector> selector(new SamplingSelector());
  auto_ptr<AnnealingSchedule> annealingSchedule;
  if (anneal) {
    annealingSchedule.reset(new LinearAnnealingSchedule(burning_its, max_temp));
    selector->SetAnnealingSchedule(annealingSchedule.get()); 
  }
  

  auto_ptr<AnnealingSchedule> detAnnealingSchedule;
  if (expected_sbleu_da) {
    detAnnealingSchedule.reset(new ExponentialAnnealingSchedule(start_temp_expda, stop_temp_expda, floor_temp_expda, anneal_ratio_da));
  }
  
  
  
  

  timer.check("Processing input file");
  while (input->HasMore()) {
    string line;
    input->GetSentence(&line, &lineno);
    cerr << "line : " << line << endl;
    if (line.empty()) {
      if (!input->HasMore()) continue;
      assert(!"I don't like empty lines");
    }
    //configure the sampler
    Sampler sampler;
    sampler.SetSelector(selector.get());
    sampler.SetCheckFeatures(checkFeatures);
    VERBOSE(2,"Reheatings: " << reheatings << endl);
    sampler.SetReheatings(reheatings);
    sampler.SetLag(lag); //thinning factor for sample collection
    auto_ptr<DerivationCollector> derivationCollector;
    auto_ptr<ExpectedLossCollector> elCollector;
    auto_ptr<GibblerMaxTransDecoder> transCollector;
    if (expected_sbleu || output_expected_sbleu) {
      elCollector.reset(new ExpectedLossCollector(gain->GetGainFunction(lineno)));
      sampler.AddCollector(elCollector.get());
    }
    else if (expected_sbleu_da) {
      elCollector.reset(new GibblerAnnealedExpectedLossCollector(gain->GetGainFunction(lineno), sampler));
      sampler.AddCollector(elCollector.get());
      //Set the annealing temperature
      int it = optimizer->GetIteration() / optimizerFreq  ;
      float temp = detAnnealingSchedule->GetTemperatureAtTime(it);
      
      GibblerAnnealedExpectedLossCollector* annealedELCollector = static_cast<GibblerAnnealedExpectedLossCollector*>(elCollector.get());
      annealedELCollector->SetTemperature(temp);
      cerr << "Annealing temperature " << annealedELCollector->GetTemperature() << endl;
    }
    if (mapdecode || decode || topn > 0 || periodic_decode > 0) {
      DerivationCollector* collector = new DerivationCollector();
      collector->setPeriodicDecode(periodic_decode);
      collector->setCollectDerivationsByTranslation(collect_dbyt);
      collector->setOutputMaxChange(output_max_change);
      derivationCollector.reset(collector);
      sampler.AddCollector(derivationCollector.get());
    }
    if (translate || mbr_decoding) {
      transCollector.reset(new GibblerMaxTransDecoder());
      transCollector->setOutputMaxChange(output_max_change);
      sampler.AddCollector(transCollector.get() );
    }
    
    MergeSplitOperator mso(merge_split_prob);
    FlipOperator fo(flip_prob);
    TranslationSwapOperator tso(retrans_prob);
    
    
    
    
    sampler.AddOperator(&mso);
    sampler.AddOperator(&tso);
    sampler.AddOperator(&fo);
    
    if (greedy || fixed_temperature == 0) {
      assert(!"greedy not supported");
    }
    else if (fixedTemp){
      assert(!"fixed temp not supported");
    }
    
    
    sampler.SetIterations(iterations);
    sampler.SetBurnIn(burning_its);

    timer.check("Running decoder");

    vector<TranslationHypothesis> translations;
    translations.push_back(TranslationHypothesis(line));
    timer.check("Running sampler");

    
    
    sampler.Run(translations,features, raoBlackwell);
    
    
    timer.check("Outputting results");

    if (expected_sbleu || expected_sbleu_da) {
      
      FVector gradient;
      FValue exp_trans_len = 0;
      FValue unreg_exp_gain = 0;
      const float exp_gain = elCollector->UpdateGradient(&gradient, &exp_trans_len, &unreg_exp_gain);
      
      (*out) << '(' << lineno << ") Expected sentence BLEU: " << exp_gain 
             << "   \tExpected length: " << exp_trans_len << endl;
      if (trainer)
        trainer->IncorporateGradient(
           exp_trans_len,
           gain->GetAverageReferenceLength(lineno),
           exp_gain,
           unreg_exp_gain,
           gradient);
    }
    if (output_expected_sbleu) {
        (*out) << "ESBLEU: " <<  lineno << " " << elCollector->getExpectedGain() << endl;
        (*out) << "EFVs: " << lineno;
        FVector scores = elCollector->getFeatureExpectations();
        (*out) << scores << endl;
    }
    
   
    
    if (derivationCollector.get()) {
      cerr << "DerivEntropy " << derivationCollector->getEntropy() << endl;
      vector<pair<const Derivation*, float> > nbest;
      derivationCollector->getNbest(nbest,max(topn,1u));
      for (size_t i = 0; i < topn && i < nbest.size() ; ++i) {  
        //const Derivation d = *(nbest[i].first);
        cerr << "NBEST: " << lineno << " ";
        derivationCollector->outputDerivationProbability(nbest[i],derivationCollector->N(),cerr);
        cerr << endl;
      }
      if (mapdecode) {
        pair<const Derivation*, float> map_soln = derivationCollector->getMAP();
        vector<string> sentence;
        map_soln.first->getTargetSentence(sentence);
        VERBOSE(1, "MAP Soln, model score [" << map_soln.second << "]" << endl)
        copy(sentence.begin(),sentence.end(),ostream_iterator<string>(*out," "));
        (*out) << endl << flush;
      }

      if (decode) {
        pair<const Derivation*, float> max = derivationCollector->getMax();
        vector<string> sentence;
        max.first->getTargetSentence(sentence);
        VERBOSE(1, "sample Soln, model score [" << max.first->getScore() << "]" << endl)
        copy(sentence.begin(),sentence.end(),ostream_iterator<string>(*out," "));
        (*out) << endl << flush;
      }
      if (collect_dbyt) {
        derivationCollector->outputDerivationsByTranslation(std::cerr);
        
      }
      if (derivation_distro) {
        std::cout << "BEGIN: derivation probability distribution" << std::endl;
        derivationCollector->printDistribution(std::cout);
        std::cout << "END: derivation probability distribution" << std::endl;
      }
      
    }
    if (translate) {
      cerr << "TransEntropy " << transCollector->getEntropy() << endl;
      pair<const Translation*,float> maxtrans = transCollector->getMax();
      (*out) << *maxtrans.first;
      (*out) << endl << flush;
      if (translation_distro) {
        std::cout << "BEGIN: translation probability distribution" << std::endl;
        transCollector->printDistribution(std::cout);
        std::cout << "END: translation probability distribution" << std::endl;
      }
      
    }
    if (mbr_decoding) {
      pair<const Translation*,float> maxtrans;
      // use samples as hyp set
        maxtrans = transCollector->getMbr(mbr_size, topNsize);  
        (*out) << *maxtrans.first;
        (*out) << endl << flush;
    }
    ++lineno;
  }
#ifdef MPI_ENABLED
  MPI_Finalize();
#endif
  (*out) << flush;
  if (!outputfile.empty())
    delete out;
  return 0;
}
