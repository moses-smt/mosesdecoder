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

#include "AnnealingSchedule.h"
#include "Decoder.h"
#include "Derivation.h"
#include "Gibbler.h"
#include "InputSource.h"
#include "TrainingSource.h"
#include "GibbsOperator.h"
#include "SentenceBleu.h"
#include "GainFunction.h"
#include "GibblerExpectedLossTraining.h"
#include "GibblerMaxTransDecoder.h"
#include "MBRDecoder.h"
#include "Model1.h"
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
  int iterations;
  unsigned int topn;
  int debug;
  int burning_its;
  int mbr_size;
  string inputfile;
  string outputfile;
  string mosesini;
  bool decode;
  bool translate;
  bool help;
  bool expected_sbleu;
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
  float eta;
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
  po::options_description desc("Allowed options");
  desc.add_options()
        ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
        ("config,f",po::value<string>(&mosesini),"Moses ini file")
        ("verbosity,v", po::value<int>(&debug)->default_value(0), "Verbosity level")
        ("random-seed,e", po::value<uint32_t>(&seed), "Random seed")
        ("timing,m", po::value(&do_timing)->zero_tokens()->default_value(false), "Display timing information.")
        ("iterations,s", po::value<int>(&iterations)->default_value(5), "Number of sampling iterations")
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
        ("eta", po::value<float>(&eta)->default_value(1.0f), "Default learning rate for SGD/EGD")
        ("mu", po::value<float>(&mu)->default_value(1.0f), "Metalearning rate for EGD")
        ("mbr", po::value(&mbr_decoding)->zero_tokens()->default_value(false), "Minimum Bayes Risk Decoding")
        ("mbr-size", po::value<int>(&mbr_size)->default_value(200),"Number of samples to use for MBR decoding")
        ("ref,r", po::value<vector<string> >(&ref_files), "Reference translation files for training")
        ("extra-feature-config,X", po::value<string>(), "Configuration file for extra (non-Moses) features");
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
  if (expected_sbleu_gradient) expected_sbleu = true;

  if (mosesini.empty()) {
      cerr << "Error: No moses ini file specified" << endl;
      return 1;
  }
  
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
  //auto_ptr<Optimizer> optimizer(new DumbStochasticGradientDescent(0.75, max_training_iterations));
  auto_ptr<Optimizer> optimizer(
    new ExponentiatedGradientDescent(
      ScoreComponentCollection(weights.size(), eta),
      mu,
      0.1f,   // minimal step scaling factor
      max_training_iterations));
  if (prior_variance != 0.0f) {
    assert(prior_variance > 0);
    std::cerr << "Using Gaussian prior: \\sigma^2=" << prior_variance << endl;
    for (size_t i = 0; i < prior_mean.size(); ++i)
      std::cerr << "  \\mu_" << i << " = " << prior_mean[i] << endl;
    optimizer->SetUseGaussianPrior(prior_mean, prior_variance);
  }
  ExpectedBleuTrainer* trainer = NULL;
  if (expected_sbleu_training) {
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
    cerr << "Reheatings: " << reheatings << endl;
    sampler.SetReheatings(reheatings);
    auto_ptr<DerivationCollector> derivationCollector;
    auto_ptr<GibblerExpectedLossCollector> elCollector;
    auto_ptr<GibblerMaxTransDecoder> transCollector;
    auto_ptr<MBRDecoder> mbrCollector;
    if (expected_sbleu) {
      elCollector.reset(new GibblerExpectedLossCollector(g[lineno]));
      sampler.AddCollector(elCollector.get());
    }
    if (decode || topn > 0 || periodic_decode > 0) {
      DerivationCollector* collector = new DerivationCollector();
      collector->setPeriodicDecode(periodic_decode);
      collector->setCollectDerivationsByTranslation(collect_dbyt);
      collector->setOutputMaxChange(output_max_change);
      derivationCollector.reset(collector);
      sampler.AddCollector(derivationCollector.get());
    }
    if (translate) {
      transCollector.reset(new GibblerMaxTransDecoder());
      transCollector->setOutputMaxChange(output_max_change);
      sampler.AddCollector(transCollector.get() );
    }
    if (mbr_decoding) {
      mbrCollector.reset(new MBRDecoder(mbr_size));
      sampler.AddCollector(mbrCollector.get() );
    }
    
    MergeSplitOperator mso;
    FlipOperator fo;
    TranslationSwapOperator tso;
    sampler.AddOperator(&mso);
    sampler.AddOperator(&tso);
    sampler.AddOperator(&fo);
    
    sampler.SetIterations(iterations);
    sampler.SetBurnIn(burning_its);
    
    TranslationOptionCollection* toc;
    Hypothesis* hypothesis;
    timer.check("Running decoder");

    std::vector<Word> source;
    decoder->decode(line,hypothesis,toc,source);
    timer.check("Running sampler");

    sampler.Run(hypothesis,toc,source,extra_features);
    timer.check("Outputting results");

    if (expected_sbleu) {
      ScoreComponentCollection gradient;
      float exp_trans_len = 0;
      const float exp_gain = elCollector->UpdateGradient(&gradient, &exp_trans_len);
      (*out) << '(' << lineno << ") Expected sentence BLEU: " << exp_gain 
             << "   \tExpected length: " << exp_trans_len << endl;
      if (trainer)
        trainer->IncorporateGradient(
           exp_trans_len,
           g[lineno].GetAverageReferenceLength(),
           exp_gain,
           gradient,
           decoder.get());
    }
    if (mbr_decoding) {
      vector<const Factor*> sentence = mbrCollector->Max();
      for (size_t i = 0; i < sentence.size(); ++i) {
        (*out) << (i > 0 ? " " : "") << *sentence[i];
      }
      (*out) << endl << flush;
      timer.check("Outputting MBR");
    }
    if (derivationCollector.get()) {
      vector<DerivationProbability> derivations;
      derivationCollector->getTopN(max(topn,1u),derivations);
      for (size_t i = 0; i < topn && i < derivations.size() ; ++i) {  
        Derivation d = *(derivations[i].first);
        cerr << "NBEST: " << lineno << " ";
        derivationCollector->outputDerivationProbability(derivations[i],cerr);
        cerr << endl;
      }
      if (decode) {
        const vector<string>& sentence = derivations[0].first->getTargetSentence();
        copy(sentence.begin(),sentence.end(),ostream_iterator<string>(*out," "));
        (*out) << endl << flush;
        
      }
      if (collect_dbyt) {
        derivationCollector->outputDerivationsByTranslation(std::cerr);
      }
    }
    if (transCollector.get()) {
      vector<const Factor*> sentence = transCollector->Max();
      for (size_t i = 0; i < sentence.size(); ++i) {
        (*out) << (i > 0 ? " " : "") << *sentence[i];
      }
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
