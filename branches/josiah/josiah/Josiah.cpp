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
#include "Dependency.h"
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
#include "Model1.h"
#include "Pos.h"
#include "Timer.h"
#include "StaticData.h"
#include "Optimizer.h"

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


void LoadReferences(const vector<string>& ref_files, GainFunctionVector* g) {
  assert(ref_files.size() > 0);
  vector<ifstream*> ifs(ref_files.size(), NULL);
  for (unsigned i = 0; i < ref_files.size(); ++i) {
    cerr << "Reference " << (i+1) << ": " << ref_files[i] << endl;
    ifs[i] = new ifstream(ref_files[i].c_str());
    assert(ifs[i]->good());
  }
  while(!ifs[0]->eof()) {
    vector<string> refs(ref_files.size());
    for (unsigned int i = 0; i < refs.size(); ++i) {
      getline(*ifs[i], refs[i]);
    }
    if (refs[0].empty() && ifs[0]->eof()) break;
    g->push_back(new SentenceBLEU(4, refs));
  }
  for (unsigned i=0; i < ifs.size(); ++i) delete ifs[i];
  cerr << "Loaded reference translations for " << g->size() << " sentences." << endl;
}

/**
  * Wrap moses timer to give a way to no-op it.
**/
class GibbsTimer {
  public:
    GibbsTimer() : m_doTiming(false) {}
    void on() {m_doTiming = true; m_timer.start("TIME: Starting timer");}
    void check(const string& msg) {if (m_doTiming) m_timer.check(string("TIME:" + msg).c_str());}
  private:
    Timer m_timer;
    bool m_doTiming;
} timer;

void configure_features_from_file(const std::string& filename, feature_vector& fv){
  std::cerr << "Reading extra features from " << filename << std::endl;
  std::ifstream in(filename.c_str());
  // todo: instead of having this function know about all required options of
  // each feature, have features populate options / read variable maps /
  // populate feature_vector using static functions.
  po::options_description desc;
  bool useApproxPef = false;
  bool useApproxPfe = false;
  bool useVerbDiff = false;
  bool useCherry = false;
  size_t dependencyFactor;
  desc.add_options()
      ("model1.table", "Model 1 table")
      ("model1.pef_column", "Column containing p(e|f) score")
      ("model1.pfe_column", "Column containing p(f|e) score")
      ("model1.approx_pef",po::value<bool>(&useApproxPef)->default_value(false), "Approximate the p(e|f), and use importance sampling")
      ("model1.approx_pfe",po::value<bool>(&useApproxPfe)->default_value(false), "Approximate the p(f|e), and use importance sampling")
      ("pos.verbdiff", po::value<bool>(&useVerbDiff)->default_value(false), "Verb difference feature")
      ("dependency.cherry", po::value<bool>(&useCherry)->default_value(false), "Use Colin Cherry's syntactic cohesiveness feature")
      ("dependency.factor", po::value<size_t>(&dependencyFactor)->default_value(1), "Factor representing the dependency tree");
  po::variables_map vm;
  po::store(po::parse_config_file(in,desc,true), vm);
  notify(vm);
  if (!vm["model1.pef_column"].empty() || !vm["model1.pfe_column"].empty()){
    boost::shared_ptr<external_model1_table> ptable;
    boost::shared_ptr<moses_factor_to_vocab_id> p_evocab_mapper;
    boost::shared_ptr<moses_factor_to_vocab_id> p_fvocab_mapper;
    if (vm["model1.table"].empty())
      throw std::runtime_error("Requesting Model 1 features, but no Model 1 table given");
    else {
      ptable.reset(new external_model1_table(vm["model1.table"].as<std::string>()));
      p_fvocab_mapper.reset(new moses_factor_to_vocab_id(ptable->f_vocab(), Moses::Input, 0, Moses::FactorCollection::Instance())); 
      p_evocab_mapper.reset(new moses_factor_to_vocab_id(ptable->e_vocab(), Moses::Output, 0, Moses::FactorCollection::Instance())); 
    }
    if (!vm["model1.pef_column"].empty()) {
      if (useApproxPef) {
        cerr << "Using approximation for model1" << endl;
        fv.push_back(feature_handle(new ApproximateModel1(ptable, p_fvocab_mapper, p_evocab_mapper)));
      } else {
        fv.push_back(feature_handle(new model1(ptable, p_fvocab_mapper, p_evocab_mapper)));
      }
    }
    if (!vm["model1.pfe_column"].empty()) {
      if (useApproxPfe) {
        cerr << "Using approximation for model1 inverse" << endl;
        fv.push_back(feature_handle(new ApproximateModel1Inverse(ptable, p_fvocab_mapper, p_evocab_mapper)));
      } else {
        fv.push_back(feature_handle(new model1_inverse(ptable, p_fvocab_mapper, p_evocab_mapper)));
      }
    }
    
  }
  if (useVerbDiff) {
      //FIXME: Should be configurable
    fv.push_back(feature_handle(new VerbDifferenceFeature(1,1)));
  }
  if (useCherry) {
    fv.push_back(feature_handle(new CherrySyntacticCohesionFeature(dependencyFactor)));
  }
  in.close();
}




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
  string stopperConfig;
  unsigned int topn;
  int debug;
  int mpidebug;
  string mpidebugfile;
  int burning_its;
  int mbr_size;
  string inputfile;
  string outputfile;
  string mosesini;
  bool decode;
  bool translate;
  bool help;
  bool expected_sbleu = false;
  bool expected_sbleu_gradient;
  bool expected_sbleu_training;
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
  ("det-annealing-ratio,A", po::value<float>(&anneal_ratio_da)->default_value(0.5f), "Deterministc annealing ratio");
  
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
  

  vector<string> featureNames;
  GetFeatureNames(&featureNames);
  

  // may be invoked just to get a features list
  if (show_features) {
    for (size_t i = 0; i < featureNames.size(); ++i)
      cout << featureNames[i] << endl;
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
  if (ref_files.size() > 0) LoadReferences(ref_files, &g);
  
  ostream* out = &cout;
  if (!outputfile.empty()) {
    ostringstream os;
    os << outputfile << '.' << rank << "_of_" << size;
    VERBOSE(1, "Writing output to: " << os.str() << endl);
    out = new ofstream(os.str().c_str());
  }
  auto_ptr<istream> in;
  auto_ptr<InputSource> input;
  
  auto_ptr<Optimizer> optimizer;

  eta.resize(weights.size());
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
    trainer = new ExpectedBleuTrainer(rank, size, training_batch_size, &input_lines, seed, randomize, optimizer.get());
    input.reset(trainer);
  } else {
    if (inputfile.size()) {
      in.reset(new ifstream(inputfile.c_str()));
      if (! *in) {
        cerr << "Error: Failed to open input file: " + inputfile << endl;
        return 1;
      }
      input.reset(new StreamInputSource(*in));
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
    if (line.empty()) {
      if (!input->HasMore()) continue;
      assert(!"I don't like empty lines");
    }
    //configure the sampler
    Sampler sampler;
    sampler.SetAnnealingSchedule(annealingSchedule.get());
    VERBOSE(2,"Reheatings: " << reheatings << endl);
    sampler.SetReheatings(reheatings);
    auto_ptr<DerivationCollector> derivationCollector;
    auto_ptr<ExpectedLossCollector> elCollector;
    auto_ptr<GibblerMaxTransDecoder> transCollector;
    if (expected_sbleu) {
      elCollector.reset(new ExpectedLossCollector(g[lineno]));
      sampler.AddCollector(elCollector.get());
    }
    else if (expected_sbleu_da) {
      elCollector.reset(new GibblerAnnealedExpectedLossCollector(g[lineno],sampler));
      sampler.AddCollector(elCollector.get());
      //Set the annealing temperature
      int it = optimizer->GetIteration() / optimizerFreq  ;
      float temp = detAnnealingSchedule->GetTemperatureAtTime(it);
      
      GibblerAnnealedExpectedLossCollector* annealedELCollector = static_cast<GibblerAnnealedExpectedLossCollector*>(elCollector.get());
      annealedELCollector->SetTemperature(temp);
      cerr << "Annealing temperature " << annealedELCollector->GetTemperature() << endl;
      
      if (temp == (static_cast<ExponentialAnnealingSchedule*>(detAnnealingSchedule.get()))->GetFloorTemp()) {//Time to start quenching
        if (initialQuenchingIteration == -1) //The iteration from which we start quenching
          initialQuenchingIteration = it;
        float quenchTemp = quenchingSchedule->GetTemperatureAtTime(it - initialQuenchingIteration + 1) ;
        cerr << "Quenching temp " <<  quenchTemp << endl;
        sampler.SetQuenchingTemperature(quenchTemp);
        if (quenchTemp >= stop_temp_quench) {
          break;
        }
      }
      else {//Use initial temperature
        sampler.SetQuenchingTemperature(start_temp_quench);
        cerr << "Using start quench temp " <<  start_temp_quench << endl;
      }
      
    }
    if (decode || topn > 0 || periodic_decode > 0) {
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
    
    MergeSplitOperator mso;
    FlipOperator fo;
    TranslationSwapOperator tso;
    sampler.AddOperator(&mso);
    sampler.AddOperator(&tso);
    sampler.AddOperator(&fo);
    
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
    sampler.Run(hypothesis,toc,source,extra_features);
    VERBOSE(1, "Language model calls: " << TranslationDelta::lmcalls << endl);
    timer.check("Outputting results");

    if (expected_sbleu || expected_sbleu_da) {
      ScoreComponentCollection gradient;
      ScoreComponentCollection hessianV;
      float exp_trans_len = 0;
      const float exp_gain = elCollector->UpdateGradient(&gradient, &exp_trans_len);
      
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
           gradient,
           decoder.get(), 
           hessianV);
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
      if (decode) {
        pair<const Derivation*, float> max = derivationCollector->getMax();
        const vector<string>& sentence = max.first->getTargetSentence();
        copy(sentence.begin(),sentence.end(),ostream_iterator<string>(*out," "));
        (*out) << endl << flush;
      }
      if (collect_dbyt) {
        derivationCollector->outputDerivationsByTranslation(std::cerr);
      }
    }
    if (translate) {
      cerr << "TransEntropy " << transCollector->getEntropy() << endl;
      pair<const Translation*,float> maxtrans = transCollector->getMax();
      (*out) << *maxtrans.first;
      (*out) << endl << flush;
    }
    if (mbr_decoding) {
      pair<const Translation*,float> maxtrans = transCollector->getMbr(mbr_size);
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
