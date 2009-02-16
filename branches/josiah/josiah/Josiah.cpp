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

#define MPI_ENABLED
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/program_options.hpp>

#include "Decoder.h"
#include "Derivation.h"
#include "Gibbler.h"
#include "GibbsOperator.h"
#include "SentenceBleu.h"
#include "GainFunction.h"
#include "GibblerExpectedLossTraining.h"
#include "GibblerMaxTransDecoder.h"
#include "MBRDecoder.h"
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

struct InputSource {
  virtual bool HasMore() const = 0;
  virtual void GetSentence(string* sentence, int* lineno) = 0;
  virtual ~InputSource() {}
};

struct StreamInputSource : public InputSource {
  string next;
  istream& in;
  StreamInputSource(istream& is) : in(is) { getline(in, next); }
  virtual bool HasMore() const {
    return !in.eof();
  }
  virtual void GetSentence(string* sentence, int* lineno) {
    (void) lineno;
    sentence->swap(next);
    getline(in, next);
  }
};


struct ExpectedBleuTrainer : public InputSource {
  ExpectedBleuTrainer(int r, int s, int bsize, vector<string>* sents, unsigned int rseed, bool randomize, Optimizer* o)
    : rank(r),
      size(s),
      batch_size(bsize),
      iteration(),
      corpus(),
      keep_going(true),
      total_exp_gain(),
      order(batch_size),
      rng(rseed),
      dist(0, sents->size() - 1),
      draw(rng, dist),
      randomize_batches(randomize),
      optimizer(o),
      total_ref_len(),
      total_exp_len() {
    if (rank >= batch_size) keep_going = false;
    corpus.swap(*sents);
    int esize = min(batch_size, size);
    //cerr << "esize: " << esize << endl;
    int sents_per_batch = batch_size / esize;
    cur = cur_start = sents_per_batch * rank;
    //cerr << "sents_per_batch: " << sents_per_batch << endl;
    cur_end = min((int)corpus.size(), sents_per_batch * (rank + 1));
    if (rank == size - 1) cur_end = batch_size;
    cerr << rank << "/" << size << ": cur_start=" << cur_start << "  cur_end=" << cur_end << endl;
    assert(cur_end >= cur_start);
    tlc = 0;
    ReserveNextBatch();
  }
  int rank, size, batch_size;
  int iteration;
  int cur, cur_start;
  int cur_end;
  vector<string> corpus;
  bool keep_going;
  float total_exp_gain;
  ScoreComponentCollection gradient;
  vector<int> order;
  boost::mt19937 rng;
  boost::uniform_smallint<int> dist;
  boost::variate_generator<boost::mt19937, boost::uniform_smallint<int> > draw;
  bool randomize_batches;
  Optimizer* optimizer;
  float total_ref_len;
  float total_exp_len;
  int tlc;
  // right now, just goes through in order, should do something
  // random
  void ReserveNextBatch() {
    if (rank == 0) {
      if (randomize_batches) {
        for (unsigned int i = 0; i < order.size(); ++i, ++tlc)
          order[i] = draw();
      } else {
        for (unsigned int i = 0; i < order.size(); ++i, ++tlc)
          order[i] = tlc % corpus.size();
      }
    } 
#ifdef MPI_ENABLED
    if (MPI_SUCCESS != MPI_Bcast(&order[0], order.size(), MPI_INT, 0, MPI_COMM_WORLD))
      MPI_Abort(MPI_COMM_WORLD,1);
#endif
  }
  virtual bool HasMore() const {
    return keep_going;
  }
  virtual void GetSentence(string* sentence, int* lineno) {
    assert(static_cast<unsigned int>(cur) < order.size());
    if (lineno) *lineno = order[cur];
    *sentence = corpus[order[cur++]];
  }
  void IncorporateGradient(
       const float trans_len,
       const float ref_len,
       const float exp_gain,
       const ScoreComponentCollection& grad,
       Decoder* decoder) {
    gradient.PlusEquals(grad);
    total_exp_gain += exp_gain;
    total_ref_len += ref_len;
    total_exp_len += trans_len;

    if (cur == cur_end) {
      vector<float> w;
      decoder->GetFeatureWeights(&w);
      ScoreComponentCollection weights(w);
      vector<float> rcv_grad(w.size());
      assert(gradient.data().size() == w.size());
      float tg = 0, trl = 0, tel = 0;
#ifdef MPI_ENABLED
      if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&gradient.data()[0]), &rcv_grad[0], w.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      if (MPI_SUCCESS != MPI_Reduce(&total_exp_gain, &tg, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      if (MPI_SUCCESS != MPI_Reduce(&total_ref_len, &trl, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      if (MPI_SUCCESS != MPI_Reduce(&total_exp_len, &tel, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
#else
      rcv_grad = gradient.data();
      tg = total_exp_gain;
      trl = total_ref_len;
      tel = total_exp_len;
#endif
      ScoreComponentCollection g(rcv_grad);
      if (rank == 0) {
        tg /= batch_size;
        g.DivideEquals(batch_size);
        cerr << "TOTAL EXPECTED GAIN: " << tg << " (batch size = " << batch_size << ")\n";
        cerr << "EXPECTED LENGTH / REF LENGTH: " << tel << '/' << trl << " (" << (tel / trl) << ")\n";
        optimizer->Optimize(tg, weights, g, &weights);
        weights.PlusEquals(g);
      }
#ifdef MPI_ENABLED
      if (MPI_SUCCESS != MPI_Bcast(const_cast<float*>(&weights.data()[0]), weights.data().size(), MPI_FLOAT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      ReserveNextBatch();
      if (MPI_SUCCESS != MPI_Barrier(MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
#endif
      decoder->SetFeatureWeights(weights.data());
      cur = cur_start;
      gradient.ZeroAll();
      total_exp_gain = 0;
      total_exp_len = 0;
      total_ref_len = 0;
      if (optimizer->HasConverged()) keep_going = false;
    }
  }
};


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
  string weightfile;
  vector<string> ref_files;
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
        ("input-file,i",po::value<string>(&inputfile),"Input file containing tokenised source")
        ("nbest-drv,n",po::value<unsigned int>(&topn)->default_value(0),"Write the top n derivations to stdout")
	("show-features,F",po::value<bool>(&show_features)->zero_tokens()->default_value(false),"Show features and then exit")
	("weights,w",po::value<string>(&weightfile),"Weight file")
        ("decode-derivation,d",po::value( &decode)->zero_tokens()->default_value(false),"Write the most likely derivation to stdout")
        ("decode-translation,t",po::value(&translate)->zero_tokens()->default_value(false),"Write the most likely translation to stdout")
        ("line-number,L", po::value(&lineno)->default_value(0), "Starting reference/line number")
        ("xbleu,x", po::value(&expected_sbleu)->zero_tokens()->default_value(false), "Compute the expected sentence BLEU")
        ("gradient,g", po::value(&expected_sbleu_gradient)->zero_tokens()->default_value(false), "Compute the gradient with respect to expected sentence BLEU")
        ("randomize-batches,R", po::value(&randomize)->zero_tokens()->default_value(false), "Randomize training batches")
        ("expected-bleu-training,T", po::value(&expected_sbleu_training)->zero_tokens()->default_value(false), "Train to maximize expected sentence BLEU")
        ("max-training-iterations,M", po::value(&max_training_iterations)->default_value(30), "Maximum training iterations")
        ("training-batch-size,S", po::value(&training_batch_size)->default_value(0), "Batch size to use during xpected bleu training, 0 = full corpus")
        ("mbr", po::value(&mbr_decoding)->zero_tokens()->default_value(false), "Minimum Bayes Risk Decoding")
        ("mbr-size", po::value<int>(&mbr_size)->default_value(200),"Number of samples to use for MBR decoding")
        ("ref,r", po::value<vector<string> >(&ref_files), "Reference translation files for training");
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
  bool print_exp_sbleu = expected_sbleu;
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
  auto_ptr<Decoder> decoder(new MosesDecoder());

  vector<string> featureNames;
  decoder->GetFeatureNames(&featureNames);

  // may be invoked just to get a features list
  if (show_features) {
    for (size_t i = 0; i < featureNames.size(); ++i)
      cout << featureNames[i] << endl;
#ifdef MPI_ENABLED
    MPI_Finalize();
#endif
    return 0;
  }
  
  //scale model weights
  vector<float> weights = StaticData::Instance().GetAllWeights();
  transform(weights.begin(),weights.end(),weights.begin(),bind2nd(multiplies<float>(),scalefactor));
  const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(weights);
  VERBOSE(1,"Scaled weights by factor of " << scalefactor << endl);
  
  

  if (vm.count("random-seed")) {
    GibbsOperator::setRandomSeed(seed);
  }      
      
  GainFunctionVector g;
  if (ref_files.size() > 0) LoadReferences(ref_files, &g);
  
  if (inputfile.empty()) {
    VERBOSE(1,"Warning: No input file specified, using toy dataset" << endl);
  }

  auto_ptr<istream> in;
  auto_ptr<InputSource> input;
  //auto_ptr<Optimizer> optimizer(new DumbStochasticGradientDescent(0.75, max_training_iterations));
  auto_ptr<Optimizer> optimizer(
    new ExponentiatedGradientDescent(
      ScoreComponentCollection(weights.size(), 1.0f),  // eta
      1.2f,  // mu, meta learning rate
      0.1f,   // minimal step scaling factor
      max_training_iterations));
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
    trainer = new ExpectedBleuTrainer(rank, size, training_batch_size, &input_lines, seed, randomize, optimizer.get());
    input.reset(trainer);
  } else {
    if (inputfile.size()) {
      in.reset(new ifstream(inputfile.c_str()));
      if (! *in) {
        cerr << "Error: Failed to open input file: " + inputfile << endl;
        return 1;
      }
    } else {
      in.reset(new istringstream(string("das parlament will das auf zweierlei weise tun .\n")));
    }
    input.reset(new StreamInputSource(*in));
  }
   
  timer.check("Processing input file");
  while (input->HasMore()) {
    string line;
    input->GetSentence(&line, &lineno);
    assert(!line.empty());
    //configure the sampler
    Sampler sampler;
    auto_ptr<DerivationCollector> derivationCollector;
    auto_ptr<GibblerExpectedLossCollector> elCollector;
    auto_ptr<GibblerMaxTransDecoder> transCollector;
    auto_ptr<MBRDecoder> mbrCollector;
    
    if (expected_sbleu) {
      elCollector.reset(new GibblerExpectedLossCollector(g[lineno]));
      sampler.AddCollector(elCollector.get());
    }
    if (decode || topn > 0) {
      derivationCollector.reset(new DerivationCollector());
      sampler.AddCollector(derivationCollector.get());
    }
    if (translate) {
      transCollector.reset(new GibblerMaxTransDecoder());
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

    decoder->decode(line,hypothesis,toc);
    timer.check("Running sampler");

    sampler.Run(hypothesis,toc);
    timer.check("Outputting results");

    if (expected_sbleu) {
      ScoreComponentCollection gradient;
      float exp_trans_len = 0;
      const float exp_gain = elCollector->UpdateGradient(&gradient, &exp_trans_len);
      (print_exp_sbleu ? cout : cerr) << '(' << lineno << ") Expected sentence BLEU: " << exp_gain << endl
                                      << "    Expected length: " << exp_trans_len << endl;
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
        cout << (i > 0 ? " " : "") << *sentence[i];
      }
      cout << endl << flush;
    }
    if (derivationCollector.get()) {
      vector<DerivationProbability> derivations;
      derivationCollector->getTopN(max(topn,1u),derivations);
      for (size_t i = 0; i < topn && i < derivations.size() ; ++i) {  
        Derivation d = *(derivations[i].first);
        cout  << lineno << " "  << std::setprecision(8) << derivations[i].second << " " << *(derivations[i].first) << endl;
      }
      if (decode) {
        const vector<string>& sentence = derivations[0].first->getTargetSentence();
        copy(sentence.begin(),sentence.end(),ostream_iterator<string>(cout," "));
        cout << endl << flush;
      }
    }
    if (transCollector.get()) {
      vector<const Factor*> sentence = transCollector->Max();
      for (size_t i = 0; i < sentence.size(); ++i) {
        cout << (i > 0 ? " " : "") << *sentence[i];
      }
      cout << endl << flush;
    }
    ++lineno;
  }
#ifdef MPI_ENABLED
  MPI_Finalize();
#endif
  return 0;
}
