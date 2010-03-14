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
#include "QuenchingSchedule.h"
#include "Decoder.h"
#include "Derivation.h"
#include "Gibbler.h"
#include "InputSource.h"
#include "TrainingSource.h"
#include "GibbsOperator.h"
#include "SentenceBleu.h"
#include "GainFunction.h"
#include "GibblerExpectedLossTraining.h"
#include "GibblerAnnealedExpectedLossTrainer.h"
#include "GibblerMaxTransDecoder.h"
#include "MpiDebug.h"
#include "StaticData.h"
#include "Optimizer.h"
#include "SampleAcceptor.h"
#include "Utils.h"
#include "KLDivergenceCalculator.h"

#if 0
  vector<string> refs;
  refs.push_back("export of high-tech products in guangdong in first two months this year reached 3.76 billion us dollars");
  refs.push_back("guangdong's export of new high technology products amounts to us $ 3.76 billion in first two months of this year");
  refs.push_back("guangdong exports us $ 3.76 billion worth of high technology products in the first two months of this year");
  refs.push_back("in the first 2 months this year , the export volume of new hi-tech products in guangdong province reached 3.76 billion us dollars .");
  SentenceBLEU sb(4, refs);
  vector<const Factor*> fv;
  GainFunction::ConvertStringToFactorArray("guangdong's new high export technology comes near on $ 3.76 million in two months of this year first", &fv);
  cerr << sb.ComputeGain(fv) << endl;
#endif
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
  string stopperConfig;
  unsigned int topn;
  int debug;
  int mpidebug;
  string mpidebugfile;
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
  bool expected_sbleu = false;
  bool expected_sbleu_gradient;
  bool expected_sbleu_training;
  bool output_expected_sbleu;
  unsigned training_batch_size;
  bool mbr_decoding;
  bool do_timing;
  bool show_features;
  int max_training_iterations;
  uint32_t seed;
  int lineno;
  bool randomize;
  float scalefactor;
  vector<float> eta;
  float mu;
  string weightfile;
  vector<string> ref_files;
  bool decode_monotone;
  bool decode_zero_weights;
  bool decode_nolm;
  bool decode_random;
  int periodic_decode;
  bool collect_dbyt;
  bool output_max_change;
  bool anneal;
  unsigned int reheatings;
  float max_temp;
  float prior_variance;
  vector<float> prior_mean;
  vector<float> prev_gradient;
  bool expected_sbleu_da;
  float start_temp_quench; 
  float stop_temp_quench;
  float start_temp_expda;
  float stop_temp_expda;  
  float floor_temp_expda;  
  float quenching_ratio;
  float anneal_ratio_da;
  float gamma;
  float lambda;
  bool use_metanormalized_egd;
  int optimizerFreq; 
  bool SMD;
  float brev_penalty_scaling_factor;
  bool hack_bp_denum;
  bool optimize_quench_temp;
  int weight_dump_freq;
  string weight_dump_stem;
  int init_iteration_number;
  bool greedy, fixedTemp;
  float fixed_temperature;
  bool collectAll, sampleCtrAll;
  bool mapdecode;
  vector<string> ngramorders;
  bool raoBlackwell;
  bool use_moses_kbesthyposet;
  bool print_moseskbest;
  bool randomScan;
  size_t lag;
  float flip_prob, merge_split_prob, retrans_prob;
  bool calcDDKL, calcTDKL;
  po::options_description desc("Allowed options");
  desc.add_options()
        ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
        ("config,f",po::value<string>(&mosesini),"Moses ini file")
        ("verbosity,v", po::value<int>(&debug)->default_value(0), "Verbosity level")
        ("mpi-debug", po::value<int>(&MpiDebug::verbosity)->default_value(0), "Verbosity level for debugging messages used in mpi.")
        ("mpi-debug-file", po::value<string>(&mpidebugfile), "Debug file stem for use by mpi processes")
        ("random-seed,e", po::value<uint32_t>(&seed), "Random seed")
        ("timing,m", po::value(&do_timing)->zero_tokens()->default_value(false), "Display timing information.")
      ("iterations,s", po::value<string>(&stopperConfig)->default_value("5"), 
       "Sampler stopping criterion (eg number of iterations)")
        ("burn-in,b", po::value<int>(&burning_its)->default_value(1), "Duration (in sampling iterations) of burn-in period")
        ("scale-factor,c", po::value<float>(&scalefactor)->default_value(1.0), "Scale factor for model weights.")
        ("decode-monotone", po::value(&decode_monotone)->zero_tokens()->default_value(false), "Run the initial decoding monotone.")
      ("decode-nolm", po::value(&decode_nolm)->zero_tokens()->default_value(false), "Run the initial decoding without an lm.")
      ("decode-zero-weights", po::value(&decode_zero_weights)->zero_tokens()->default_value(false), "Run the initial decoding with weights set to zero.")
      ("decode-random", po::value(&decode_random)->zero_tokens()->default_value(false), "Use the random decoder.")
        ("input-file,i",po::value<string>(&inputfile),"Input file containing tokenised source")
        ("output-file-prefix,o",po::value<string>(&outputfile),"Output file prefix for translations, MBR output, etc")
        ("nbest-drv,n",po::value<unsigned int>(&topn)->default_value(0),"Write the top n derivations to stdout")
	("show-features,F",po::value<bool>(&show_features)->zero_tokens()->default_value(false),"Show features and then exit")
	("weights,w",po::value<string>(&weightfile),"Weight file")
        ("decode-derivation,d",po::value( &decode)->zero_tokens()->default_value(false),"Write the most likely derivation to stdout")
        ("decode-translation,t",po::value(&translate)->zero_tokens()->default_value(false),"Write the most likely translation to stdout")
      ("distro-derivation", po::value(&derivation_distro)->zero_tokens()->default_value(false), "Print derivation probability distribution")
      ("distro-translation", po::value(&translation_distro)->zero_tokens()->default_value(false), "Print translation probability distribution")
        ("periodic-derivation,p",po::value(&periodic_decode)->default_value(0), "Periodically write the max derivation to stderr")
      ("max-change", po::value(&output_max_change)->zero_tokens()->default_value(false), "Whenever the max deriv or max trans changes, write it to stderr")
        ("collect-dbyt",po::value(&collect_dbyt)->zero_tokens()->default_value(false), "Collect derivations per translation")
        ("line-number,L", po::value(&lineno)->default_value(0), "Starting reference/line number")
        ("xbleu,x", po::value(&expected_sbleu)->zero_tokens()->default_value(false), "Compute the expected sentence BLEU")
        ("gradient,g", po::value(&expected_sbleu_gradient)->zero_tokens()->default_value(false), "Compute the gradient with respect to expected sentence BLEU")
        ("randomize-batches,R", po::value(&randomize)->zero_tokens()->default_value(false), "Randomize training batches")
        ("gaussian-prior-variance", po::value<float>(&prior_variance)->default_value(0.0f), "Gaussian prior variance (0 for no prior)")
        ("gaussian-prior-mean,P", po::value<vector<float> >(&prior_mean), "Gaussian prior means")
        ("expected-bleu-training,T", po::value(&expected_sbleu_training)->zero_tokens()->default_value(false), "Train to maximize expected sentence BLEU")
          ("output-expected-sbleu", po::value(&output_expected_sbleu)->zero_tokens()->default_value(false), "Output expected bleu and feature expectations at end of sampling")
        ("max-training-iterations,M", po::value(&max_training_iterations)->default_value(30), "Maximum training iterations")
        ("training-batch-size,S", po::value(&training_batch_size)->default_value(0), "Batch size to use during xpected bleu training, 0 = full corpus")
	("reheatings", po::value<unsigned int>(&reheatings)->default_value(1), "Number of times to reheat the sampler")
	("anneal,a", po::value(&anneal)->default_value(false)->zero_tokens(), "Use annealing during the burn in period")
	("max-temp", po::value<float>(&max_temp)->default_value(4.0), "Annealing maximum temperature")
        ("eta", po::value<vector<float> >(&eta), "Default learning rate for SGD/EGD")
        ("prev-gradient", po::value<vector<float> >(&prev_gradient), "Previous gradient for restarting SGD/EGD")
        ("mu", po::value<float>(&mu)->default_value(1.0f), "Metalearning rate for EGD")
        ("gamma", po::value<float>(&gamma)->default_value(0.9f), "Smoothing parameter for Metanormalized EGD ")
        ("lambda", po::value<float>(&lambda)->default_value(1.0f), "Smoothing parameter for SMD ")
        ("mbr", po::value(&mbr_decoding)->zero_tokens()->default_value(false), "Minimum Bayes Risk Decoding")
        ("mbr-size", po::value<int>(&mbr_size)->default_value(200),"Number of samples to use for MBR decoding")
        ("topn-size", po::value<int>(&topNsize)->default_value(0),"Number of samples to use for inner loop of MBR decoding")
        ("ref,r", po::value<vector<string> >(&ref_files), "Reference translation files for training")
        ("extra-feature-config,X", po::value<string>(), "Configuration file for extra (non-Moses) features")
        ("use-metanormalized-egd,N", po::value(&use_metanormalized_egd)->zero_tokens()->default_value(false), "Use metanormalized EGD")
        ("use-SMD", po::value(&SMD)->zero_tokens()->default_value(false), "Train using SMD") 
        ("expected-bleu-deterministic-annealing-training,D", po::value(&expected_sbleu_da)->zero_tokens()->default_value(false), "Train to maximize expected sentence BLEU using deterministic annealing")   
        ("optimizer-freq", po::value<int>(&optimizerFreq)->default_value(1),"Number of optimization to perform at given temperature")
        ("initial-quenching-temp", po::value<float>(&start_temp_quench)->default_value(1.0f), "Initial quenching temperature")
        ("final-quenching-temp", po::value<float>(&stop_temp_quench)->default_value(200.0f), "Final quenching temperature")
        ("quenching-ratio,Q", po::value<float>(&quenching_ratio)->default_value(2.0f), "Quenching ratio")
   ("initial-det-anneal-temp", po::value<float>(&start_temp_expda)->default_value(1000.0f), "Initial deterministic annealing entropy temperature")
   ("final-det-anneal-temp", po::value<float>(&stop_temp_expda)->default_value(0.001f), "Final deterministic annealing entropy temperature")
   ("floor-temp", po::value<float>(&floor_temp_expda)->default_value(0.0f), "Floor temperature for det annealing")
  ("det-annealing-ratio,A", po::value<float>(&anneal_ratio_da)->default_value(0.5f), "Deterministc annealing ratio")
  ("hack-bp-denum,H", po::value(&hack_bp_denum)->default_value(false), "Use a predefined scalar as denum in BP computation")
  ("bp-scale,B", po::value<float>(&brev_penalty_scaling_factor)->default_value(1.0f), "Scaling factor for sent level brevity penalty for BLEU - default is 1.0")
  ("optimize-quench-temp", po::value(&optimize_quench_temp)->zero_tokens()->default_value(false), "Optimizing quenching temp during annealing")
      ("weight-dump-freq", po::value<int>(&weight_dump_freq)->default_value(0), "Frequency to dump weight files during training")
  ("weight-dump-stem", po::value<string>(&weight_dump_stem)->default_value("weights"), "Stem of filename to use for dumping weights")
      ("init-iteration-number", po::value<int>(&init_iteration_number)->default_value(0), "First training iteration will be one after this (useful for restarting)")
 ("greedy", po::value(&greedy)->zero_tokens()->default_value(false), "Greedy sample acceptor")
  ("fixed-temp-accept", po::value(&fixedTemp)->zero_tokens()->default_value(false), "Fixed temperature sample acceptor")
  ("fixed-temperature", po::value<float>(&fixed_temperature)->default_value(1.0f), "Temperature for fixed temp sample acceptor")
  ("collect-all", po::value(&collectAll)->zero_tokens()->default_value(false), "Collect all samples generated")
  ("sample-ctr-all", po::value(&sampleCtrAll)->zero_tokens()->default_value(false), "When in CollectAllSamples model, increment collection ctr after each sample has been collected")
          ("rao-blackwell", po::value(&raoBlackwell)->zero_tokens()->default_value(false), "Do Rao-Blackwellisation (aka conditional estimation")
  ("mapdecode", po::value(&mapdecode)->zero_tokens()->default_value(false), "MAP decoding")
  ("mh.ngramorders", po::value< vector <string> >(&ngramorders), "Indicate LMs and ngram orders to be used during MH/Gibbs")
  ("use-moses-kbesthyposet", po::value(&use_moses_kbesthyposet)->zero_tokens()->default_value(false), "Use Moses to generate kbest hypothesis set")
  ("print-moseskbest", po::value(&print_moseskbest)->zero_tokens()->default_value(false), "Print Moses kbest")
  ("random-scan", po::value(&randomScan)->zero_tokens()->default_value(false), "Sample using random scan")
  ("lag", po::value<size_t>(&lag)->default_value(10), "Lag between collecting samples")
  ("flip-prob", po::value<float>(&flip_prob)->default_value(0.6f), "Probability of applying flip operator during random scan")
  ("merge-split-prob", po::value<float>(&merge_split_prob)->default_value(0.2f), "Probability of applying merge-split operator during random scan")
  ("retrans-prob", po::value<float>(&retrans_prob)->default_value(0.2f), "Probability of applying retrans operator during random scan")
  ("calc-ddistro-kl", po::value(&calcDDKL)->zero_tokens()->default_value(false), "Calculate KL divergence between sampler estimated derivation distro and true")
  ("calc-tdistro-kl", po::value(&calcTDKL)->zero_tokens()->default_value(false), "Calculate KL divergence between sampler estimated translation distro and true");
 
  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);

  feature_vector extra_features; 
  if (!vm["extra-feature-config"].empty()){
    configure_features_from_file(vm["extra-feature-config"].as<std::string>(), extra_features);
  }
  std::cerr << "Using " << extra_features.size() << " extra features" << std::endl;

  if (help) {
      std::cout << "Usage: " + string(argv[0]) +  " -f mosesini-file [options]" << std::endl;
      std::cout << desc << std::endl;
      return 0;
  }

  if (expected_sbleu_training && expected_sbleu_da) {
    std::cerr << "Incorrect usage: Cannot do both expected bleu training and expected bleu deterministic annealing training" << std::endl;
    return 0;
  }
  
  if (randomScan) {
    float opProb = flip_prob + merge_split_prob + retrans_prob;
    if (fabs(1.0 - opProb) > 0.00001) {
      std::cerr << "Incorrect usage: specified operator probs should sum up to 1" << std::endl;
      return 0;  
    }
  }
  
  
  if (translation_distro) translate = true;
  if (derivation_distro) decode = true;
  bool defaultCtrIncrementer = !sampleCtrAll;
  
  expected_sbleu = false;
  if (expected_sbleu_gradient == true && expected_sbleu_da == false) expected_sbleu = true;
  
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
  
   //set up moses
  initMoses(mosesini,weightfile,debug);
  auto_ptr<Decoder> decoder;
  if (decode_random) {
    if (decode_monotone || decode_zero_weights || decode_nolm) {
      cerr << "Error:: Random decoder cannot be used with any other options." << endl;
#ifdef MPI_ENABLED
      MPI_Finalize();
#endif
      return -1;
    }
    decoder.reset(new RandomDecoder());
  } else {
    decoder.reset(new MosesDecoder());
  }
  
  
  auto_ptr<MHAcceptor> mhAcceptor;
  bool doMH = false;
  
  
  //Initialise LMs info
  map <LanguageModel*, int> proposalLMInfo, targetLMInfo;
  
  const LMList& languageModels = StaticData::Instance().GetAllLM();
  for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {  
    LanguageModel* lm = *i; 
    targetLMInfo[lm] = lm->GetNGramOrder();
  }
  proposalLMInfo = targetLMInfo;
  if (ngramorders.size()) { //MH info
    mhAcceptor.reset(new MHAcceptor());
    mhAcceptor.get()->setTargetLMInfo(targetLMInfo);
    bool success = false;
    for (size_t i = 0; i < ngramorders.size(); ++i) {
      vector <string> tokens = Tokenize(ngramorders[i], ":");
      assert (tokens.size() == 2);
      string name = tokens[0];
      int order = atoi(tokens[1].c_str());
      LanguageModel* lm;
      success = ValidateAndGetLMFromName(name, lm);
      if (success) {
        doMH = true;
        proposalLMInfo[lm] = order;
        mhAcceptor.get()->addScoreProducer(lm); 
      }
      else {
        cerr << "Error: That feature does not exist. " << endl;
#ifdef MPI_ENABLED
        MPI_Finalize();
#endif
        return -1;
      }
    }
    if (success) {
      mhAcceptor.get()->setProposalLMInfo(proposalLMInfo);  
    } 
  }
  
  
  
  // may be invoked just to get a features list
  if (show_features) {
    OutputWeights(cout);
#ifdef MPI_ENABLED
    MPI_Finalize();
#endif
    return 0;
  }
  
  
  
  if (decode_monotone) {
    decoder->SetMonotone(true);
  }
  
  if (decode_zero_weights) {
    decoder->SetZeroWeights(true);
  }
  
  if (decode_nolm) {
    decoder->SetNoLM(true);
  }
  
  //scale model weights
  vector<float> weights = StaticData::Instance().GetAllWeights();
  transform(weights.begin(),weights.end(),weights.begin(),bind2nd(multiplies<float>(),scalefactor));
  const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(weights);
  VERBOSE(1,"Scaled weights by factor of " << scalefactor << endl);
  
  

  if (vm.count("random-seed")) {
    RandomNumberGenerator::instance().setSeed(seed + rank);
  }      
      
  GainFunctionVector g;
  if (ref_files.size() > 0) LoadReferences(ref_files, inputfile, &g, brev_penalty_scaling_factor, hack_bp_denum);
  
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

  if (optimize_quench_temp) {
   eta.resize(weights.size()+1); 
   eta[eta.size()-1] = eta[0]; //Set quenching eta 
  }
  else {
   eta.resize(weights.size());   
  }
  
  
  prev_gradient.resize(weights.size());

  if (use_metanormalized_egd) {
    optimizer.reset(new MetaNormalizedExponentiatedGradientDescent(
                                                             ScoreComponentCollection(eta),
                                                             mu,
                                                             0.1f,   // minimal step scaling factor
                                                             gamma,                                       
                                                             max_training_iterations,
                                                             ScoreComponentCollection(prev_gradient)));
  }
  else if (SMD) {
    optimizer.reset(new StochasticMetaDescent(
                                                                   ScoreComponentCollection(eta),
                                                                   mu,
                                                                   0.5f,   // minimal step scaling factor
                                                                   lambda,                                       
                                                                   max_training_iterations,
                                                                   ScoreComponentCollection(prev_gradient)));
  }
  else {
    optimizer.reset(new ExponentiatedGradientDescent(
                                                                   ScoreComponentCollection(eta),
                                                                   mu,
                                                                   0.1f,   // minimal step scaling factor
                                                                   max_training_iterations,
                                                                   ScoreComponentCollection(prev_gradient)));
  }
  if (optimizer.get()) {
      optimizer->SetIteration(init_iteration_number);
  }
  if (prior_variance != 0.0f) {
    assert(prior_variance > 0);
    std::cerr << "Using Gaussian prior: \\sigma^2=" << prior_variance << endl;
    for (size_t i = 0; i < prior_mean.size(); ++i)
      std::cerr << "  \\mu_" << i << " = " << prior_mean[i] << endl;
    optimizer->SetUseGaussianPrior(prior_mean, prior_variance);
  }
  ExpectedBleuTrainer* trainer = NULL;
  if (expected_sbleu_training || expected_sbleu_da) {
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
                                      init_iteration_number, weight_dump_freq, weight_dump_stem);
    input.reset(trainer);
  } else {
    if (inputfile.size()) {
      input.reset(new BatchedFileInputSource(inputfile,rank,size));
    } else {
      input.reset(new StreamInputSource(cin));
    }
  }
   
  auto_ptr<AnnealingSchedule> annealingSchedule;
  if (anneal) {
    annealingSchedule.reset(new LinearAnnealingSchedule(burning_its, max_temp));  
  }

  auto_ptr<QuenchingSchedule> quenchingSchedule;
  auto_ptr<AnnealingSchedule> detAnnealingSchedule;
  if (expected_sbleu_da) {
    quenchingSchedule.reset(new ExponentialQuenchingSchedule(start_temp_quench, stop_temp_quench, quenching_ratio));
    detAnnealingSchedule.reset(new ExponentialAnnealingSchedule(start_temp_expda, stop_temp_expda, floor_temp_expda, anneal_ratio_da));
  }
  
  int initialQuenchingIteration = -1;

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
    sampler.SetAnnealingSchedule(annealingSchedule.get());
    VERBOSE(2,"Reheatings: " << reheatings << endl);
    sampler.SetReheatings(reheatings);
    sampler.SetRandomScan(randomScan); //random or sequential scan
    sampler.SetLag(lag); //thinning factor for sample collection
    auto_ptr<DerivationCollector> derivationCollector;
    auto_ptr<ExpectedLossCollector> elCollector;
    auto_ptr<GibblerMaxTransDecoder> transCollector;
    if (expected_sbleu || output_expected_sbleu) {
      elCollector.reset(new ExpectedLossCollector(&(g[lineno])));
      sampler.AddCollector(elCollector.get());
    }
    else if (expected_sbleu_da) {
      elCollector.reset(new GibblerAnnealedExpectedLossCollector(&(g[lineno]), sampler));
      sampler.AddCollector(elCollector.get());
      //Set the annealing temperature
      int it = optimizer->GetIteration() / optimizerFreq  ;
      float temp = detAnnealingSchedule->GetTemperatureAtTime(it);
      
      GibblerAnnealedExpectedLossCollector* annealedELCollector = static_cast<GibblerAnnealedExpectedLossCollector*>(elCollector.get());
      annealedELCollector->SetTemperature(temp);
      cerr << "Annealing temperature " << annealedELCollector->GetTemperature() << endl;
      
      if (optimizer->GetIteration() == 0 || !optimize_quench_temp) {
        sampler.SetQuenchingTemperature(start_temp_quench);  
      }
      
      if (temp == (static_cast<ExponentialAnnealingSchedule*>(detAnnealingSchedule.get()))->GetFloorTemp()) {//Time to start quenching
        if (initialQuenchingIteration == -1) {//The iteration from which we start quenching          
          initialQuenchingIteration = it;
          //Do not optimize quenching temp when quenching
          annealedELCollector->SetComputeScaleGradient(false);
          trainer->SetComputeScaleGradient(false);
          //Resize optimizer's fields
          if (optimize_quench_temp) {
            static_cast<ExponentiatedGradientDescent*>(optimizer.get())->ResizeEta();
            static_cast<ExponentiatedGradientDescent*>(optimizer.get())->ResizePreviousGradient();
          }
        } 
          
        float quenchTemp = start_temp_quench;
        if (optimize_quench_temp) { 
          quenchTemp = trainer->GetCurrQuenchingTemp();
        }  
        assert (quenchTemp > 0);
        
        quenchTemp *= quenchingSchedule->GetTemperatureAtTime(it - initialQuenchingIteration) ;
        sampler.SetQuenchingTemperature(quenchTemp);
        if (quenchTemp >= stop_temp_quench) {
          break;
        }
      }
      else {
        //Do compute scaling gradient when cooling
        if (optimize_quench_temp) {
          if (optimizer->GetIteration() > 0) 
            sampler.SetQuenchingTemperature(trainer->GetCurrQuenchingTemp());  
          annealedELCollector->SetComputeScaleGradient(true);
          trainer->SetComputeScaleGradient(true);  
        }
      }
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
    
    MergeSplitOperator mso(randomScan, merge_split_prob);
    FlipOperator fo(randomScan, flip_prob);
    TranslationSwapOperator tso(randomScan, retrans_prob);
    mso.setGibbsLMInfo(targetLMInfo);
    fo.setGibbsLMInfo(targetLMInfo);
    tso.setGibbsLMInfo(targetLMInfo);
    
    if (proposalLMInfo.size()) {
      mso.setGibbsLMInfo(proposalLMInfo);
      mso.addMHAcceptor(mhAcceptor.get());
      tso.setGibbsLMInfo(proposalLMInfo);
      tso.addMHAcceptor(mhAcceptor.get()); 
    }
    
    sampler.AddOperator(&mso);
    sampler.AddOperator(&tso);
    sampler.AddOperator(&fo);
    
    //Acceptor
    auto_ptr<SampleAcceptor> acceptor;
    if (greedy || fixed_temperature == 0) {
      acceptor.reset(new GreedyAcceptor());
    }
    else if (fixedTemp){
      acceptor.reset(new FixedTempAcceptor(fixed_temperature));
    }
    else {
      acceptor.reset(new RegularAcceptor);
    }
    
    
    //sampler stopping strategy; TODO: push parsing of config into StoppingStrategy ctor ?
    auto_ptr<StopStrategy> stopper;
    try {
      size_t iterations = lexical_cast<size_t>(stopperConfig);
      stopper.reset(new CountStopStrategy(iterations));
    } catch (bad_lexical_cast&) {/* do nothing*/}
    
    if (!stopper.get()) {
      //non-numeric. Try to pass the config string.
      static string maxderiv = "maxderiv:";
      bool useMaxderiv = false;
      static string maxtrans = "maxtrans:";
      bool useMaxtrans = false;
      if (stopperConfig.find(maxderiv) == 0) useMaxderiv = true;
      if (stopperConfig.find(maxtrans) == 0) useMaxtrans = true;
      if (useMaxderiv || useMaxtrans) {
        //format is (maxtrans|maxderiv):<miniters>:<maxiters>:<maxcount>
        std::vector<std::string> fields;
        boost::split(fields, stopperConfig, boost::is_any_of(":"));
        if (fields.size() == 4) {
          size_t miniters =  lexical_cast<size_t>(fields[1]);
          size_t maxiters =  lexical_cast<size_t>(fields[2]);
          size_t maxcount =  lexical_cast<size_t>(fields[3]);
          if (useMaxderiv) {
            if (!derivationCollector.get()) {
              cerr << "Error: Need to be collecting derivations to use this stop strategy" << endl;
              exit (1);
            }
            stopper.reset(new MaxCountStopStrategy<Derivation>(miniters, maxiters, maxcount, derivationCollector.get()));
          }
          if (useMaxtrans) {
            if (!transCollector.get()) {
              cerr << "Error: Need to be collecting translations to use this stop strategy" << endl;
              exit (1);
            }
            stopper.reset(new MaxCountStopStrategy<Translation>(miniters, maxiters, maxcount, transCollector.get()));
          }
        }
      }
    }
    
    if (!stopper.get()) {
      cerr << "Error: unable to parse stopper config string '" << stopperConfig << "'" << endl;
      exit(1);
    }
    
    sampler.SetStopper(stopper.get());
    sampler.SetBurnIn(burning_its);
    Hypothesis* hypothesis;
    TranslationOptionCollection* toc;

    timer.check("Running decoder");

    std::vector<Word> source;
    decoder->decode(line,hypothesis,toc,source);
    timer.check("Running sampler");

    TranslationDelta::lmcalls = 0;
    
    if (doMH) {
      MHAcceptor::mhtotal = 0;  
      MHAcceptor::acceptanceCtr = 0;  
    }
    
    sampler.Run(hypothesis,toc,source,extra_features, acceptor.get(), collectAll, defaultCtrIncrementer,raoBlackwell);  
    VERBOSE(1, "Language model calls: " << TranslationDelta::lmcalls << endl);
    if (doMH) {
      VERBOSE(0, "Total number of Metropolis-Hastings Steps :" << MHAcceptor::mhtotal << endl) 
      VERBOSE(0, "Total number of accepted Metropolis-Hastings Steps :" <<  MHAcceptor::acceptanceCtr << endl) 
    }
    
    timer.check("Outputting results");

    if (expected_sbleu || expected_sbleu_da) {
      
      ScoreComponentCollection gradient;
      ScoreComponentCollection hessianV;
      float exp_trans_len = 0;
      float unreg_exp_gain = 0;
      float scaling_gradient = 0;
      const float exp_gain = elCollector->UpdateGradient(&gradient, &exp_trans_len, &unreg_exp_gain, &scaling_gradient);
      
      if (SMD) {
        const ScoreComponentCollection& v =  static_cast<StochasticMetaDescent*>(optimizer.get())->GetV();
        elCollector->UpdateHessianVProduct(&hessianV, v);
      }
      (*out) << '(' << lineno << ") Expected sentence BLEU: " << exp_gain 
             << "   \tExpected length: " << exp_trans_len << endl;
      if (trainer)
        trainer->IncorporateGradient(
           exp_trans_len,
           g[lineno].GetAverageReferenceLength(),
           exp_gain,
           unreg_exp_gain,
           gradient,
           decoder.get(), 
           hessianV, 
           scaling_gradient);
    }
    if (output_expected_sbleu) {
        (*out) << "ESBLEU: " <<  lineno << " " << elCollector->getExpectedGain() << endl;
        (*out) << "EFVs: " << lineno;
        ScoreComponentCollection scores = elCollector->getFeatureExpectations();
        for (size_t i = 0; i < scores.size(); ++i) {
            (*out) << " " << scores[i];
        }
        (*out) << endl;
    }
    
    auto_ptr<KLDivergenceCalculator> klCalculator; //to calculate KL Divergence
    if (calcDDKL || calcTDKL){
      klCalculator.reset(new KLDivergenceCalculator(line));
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
        VERBOSE(0, "MAP Soln, model score [" << map_soln.second << "]" << endl)
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
      if (calcDDKL) {
        vector<pair<const Derivation*, float> > nbest;
        derivationCollector->getNbest(nbest,max(topn,0u));
        float ddDivergence = klCalculator->calcDDDivergence(nbest);
        std::cout << "Derivation distribution divergence, for n=" << nbest.size() << " is: " << ddDivergence << std::endl;
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
      if (calcTDKL) {
        vector<pair<const Translation*, float> > nbest;
        transCollector->getNbest(nbest,max(topn,0u));
        float tdDivergence = klCalculator->calcTDDivergence(nbest);
        std::cout << "Translation distribution divergence, for n=" << nbest.size() << " is: " << tdDivergence << std::endl;
      }
    }
    if (mbr_decoding) {
      pair<const Translation*,float> maxtrans;
      if (use_moses_kbesthyposet) { //use moses kbest as hypothesis set
        //let's run the decoder to get the hyp set
        MosesDecoder moses;
        Hypothesis* hypothesis;
        TranslationOptionCollection* toc;
        std::vector<Word> source;
        
        timer.check("Running decoder");
        moses.decode(line,hypothesis,toc,source, mbr_size);
        
        if (print_moseskbest) {
          moses.PrintNBest(std::cout);
        }
        
        const std::vector<pair<Translation, float > > &  translations = moses.GetNbestTranslations(); 
        size_t maxtransIndex = transCollector->getMbr(translations, topNsize);  
        (*out) << translations[maxtransIndex].first;
        (*out) << endl << flush;
      }
      else {// use samples as hyp set
        maxtrans = transCollector->getMbr(mbr_size, topNsize);  
        (*out) << *maxtrans.first;
        (*out) << endl << flush;
      }
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
