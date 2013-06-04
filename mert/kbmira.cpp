// $Id$
// vim:tabstop=2
/***********************************************************************
K-best Batch MIRA for Moses
Copyright (C) 2012, National Research Council Canada / Conseil national
de recherches du Canada
***********************************************************************/

/**
  * k-best Batch Mira, as described in:
  *
  * Colin Cherry and George Foster
  * Batch Tuning Strategies for Statistical Machine Translation
  * NAACL 2012
  *
  * Implemented by colin.cherry@nrc-cnrc.gc.ca
  *
  * To license implementations of any of the other tuners in that paper,
  * please get in touch with any member of NRC Canada's Portage project
  *
  * Input is a set of n-best lists, encoded as feature and score files.
  *
  * Output is a weight file that results from running MIRA on these
  * n-btest lists for J iterations. Will return the set that maximizes
  * training BLEU.
 **/

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "BleuScorer.h"
#include "HypPackEnumerator.h"
#include "MiraFeatureVector.h"
#include "MiraWeightVector.h"

using namespace std;
using namespace MosesTuning;

namespace po = boost::program_options;

ValType evaluate(HypPackEnumerator* train, const AvgWeightVector& wv)
{
  vector<ValType> stats(kBleuNgramOrder*2+1,0);
  for(train->reset(); !train->finished(); train->next()) {
    // Find max model
    size_t max_index=0;
    ValType max_score=0;
    for(size_t i=0; i<train->cur_size(); i++) {
      MiraFeatureVector vec(train->featuresAt(i));
      ValType score = wv.score(vec);
      if(i==0 || score > max_score) {
        max_index = i;
        max_score = score;
      }
    }
    // Update stats
    const vector<float>& sent = train->scoresAt(max_index);
    for(size_t i=0; i<sent.size(); i++) {
      stats[i]+=sent[i];
    }
  }
  return unsmoothedBleu(stats);
}

int main(int argc, char** argv)
{
  const ValType BLEU_RATIO = 5;
  bool help;
  string denseInitFile;
  string sparseInitFile;
  vector<string> scoreFiles;
  vector<string> featureFiles;
  int seed;
  string outputFile;
  float c = 0.01;      // Step-size cap C
  float decay = 0.999; // Pseudo-corpus decay \gamma
  int n_iters = 60;    // Max epochs J
  bool streaming = false; // Stream all k-best lists?
  bool no_shuffle = false; // Don't shuffle, even for in memory version
  bool model_bg = false; // Use model for background corpus
  bool verbose = false; // Verbose updates
  bool safe_hope = false; // Model score cannot have more than BLEU_RATIO times more influence than BLEU

  // Command-line processing follows pro.cpp
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("scfile,S", po::value<vector<string> >(&scoreFiles), "Scorer data files")
  ("ffile,F", po::value<vector<string> > (&featureFiles), "Feature data files")
  ("random-seed,r", po::value<int>(&seed), "Seed for random number generation")
  ("output-file,o", po::value<string>(&outputFile), "Output file")
  ("cparam,C", po::value<float>(&c), "MIRA C-parameter, lower for more regularization (default 0.01)")
  ("decay,D", po::value<float>(&decay), "BLEU background corpus decay rate (default 0.999)")
  ("iters,J", po::value<int>(&n_iters), "Number of MIRA iterations to run (default 60)")
  ("dense-init,d", po::value<string>(&denseInitFile), "Weight file for dense features")
  ("sparse-init,s", po::value<string>(&sparseInitFile), "Weight file for sparse features")
  ("streaming", po::value(&streaming)->zero_tokens()->default_value(false), "Stream n-best lists to save memory, implies --no-shuffle")
  ("no-shuffle", po::value(&no_shuffle)->zero_tokens()->default_value(false), "Don't shuffle hypotheses before each epoch")
  ("model-bg", po::value(&model_bg)->zero_tokens()->default_value(false), "Use model instead of hope for BLEU background")
  ("verbose", po::value(&verbose)->zero_tokens()->default_value(false), "Verbose updates")
  ("safe-hope", po::value(&safe_hope)->zero_tokens()->default_value(false), "Mode score's influence on hope decoding is limited")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);
  if (help) {
    cout << "Usage: " + string(argv[0]) +  " [options]" << endl;
    cout << desc << endl;
    exit(0);
  }

  cerr << "kbmira with c=" << c << " decay=" << decay << " no_shuffle=" << no_shuffle << endl;

  if (vm.count("random-seed")) {
    cerr << "Initialising random seed to " << seed << endl;
    srand(seed);
  } else {
    cerr << "Initialising random seed from system clock" << endl;
    srand(time(NULL));
  }

  // Initialize weights
  ///
  // Dense
  vector<parameter_t> initParams;
  if(!denseInitFile.empty()) {
    ifstream opt(denseInitFile.c_str());
    string buffer;
    if (opt.fail()) {
      cerr << "could not open dense initfile: " << denseInitFile << endl;
      exit(3);
    }
    parameter_t val;
    getline(opt,buffer);
    istringstream strstrm(buffer);
    while(strstrm >> val) {
      initParams.push_back(val);
    }
    opt.close();
  }
  size_t initDenseSize = initParams.size();
  // Sparse
  if(!sparseInitFile.empty()) {
    if(initDenseSize==0) {
      cerr << "sparse initialization requires dense initialization" << endl;
      exit(3);
    }
    ifstream opt(sparseInitFile.c_str());
    if(opt.fail()) {
      cerr << "could not open sparse initfile: " << sparseInitFile << endl;
      exit(3);
    }
    int sparseCount=0;
    parameter_t val;
    std::string name;
    while(opt >> name >> val) {
      size_t id = SparseVector::encode(name) + initDenseSize;
      while(initParams.size()<=id) initParams.push_back(0.0);
      initParams[id] = val;
      sparseCount++;
    }
    cerr << "Found " << sparseCount << " initial sparse features" << endl;
    opt.close();
  }

  MiraWeightVector wv(initParams);

  // Initialize background corpus
  vector<ValType> bg;
  for(int j=0; j<kBleuNgramOrder; j++) {
    bg.push_back(kBleuNgramOrder-j);
    bg.push_back(kBleuNgramOrder-j);
  }
  bg.push_back(kBleuNgramOrder);

  // Training loop
  boost::scoped_ptr<HypPackEnumerator> train;
  if(streaming)
    train.reset(new StreamingHypPackEnumerator(featureFiles, scoreFiles));
  else
    train.reset(new RandomAccessHypPackEnumerator(featureFiles, scoreFiles, no_shuffle));
  cerr << "Initial BLEU = " << evaluate(train.get(), wv.avg()) << endl;
  ValType bestBleu = 0;
  for(int j=0; j<n_iters; j++) {
    // MIRA train for one epoch
    int iNumHyps = 0;
    int iNumExamples = 0;
    int iNumUpdates = 0;
    ValType totalLoss = 0.0;
    for(train->reset(); !train->finished(); train->next()) {
      // Hope / fear decode
      ValType hope_scale = 1.0;
      size_t hope_index=0, fear_index=0, model_index=0;
      ValType hope_score=0, fear_score=0, model_score=0;
      int iNumHypsBackup = iNumHyps;
      for(size_t safe_loop=0; safe_loop<2; safe_loop++) {
        iNumHyps = iNumHypsBackup;
        ValType hope_bleu, hope_model;
        for(size_t i=0; i< train->cur_size(); i++) {
          const MiraFeatureVector& vec=train->featuresAt(i);
          ValType score = wv.score(vec);
          ValType bleu = sentenceLevelBackgroundBleu(train->scoresAt(i),bg);
          // Hope
          if(i==0 || (hope_scale*score + bleu) > hope_score) {
            hope_score = hope_scale*score + bleu;
            hope_index = i;
            hope_bleu = bleu;
            hope_model = score;
          }
          // Fear
          if(i==0 || (score - bleu) > fear_score) {
            fear_score = score - bleu;
            fear_index = i;
          }
          // Model
          if(i==0 || score > model_score) {
            model_score = score;
            model_index = i;
          }
          iNumHyps++;
        }
        // Outer loop rescales the contribution of model score to 'hope' in antagonistic cases
        // where model score is having far more influence than BLEU
        hope_bleu *= BLEU_RATIO; // We only care about cases where model has MUCH more influence than BLEU
        if(safe_hope && safe_loop==0 && abs(hope_model)>1e-8 && abs(hope_bleu)/abs(hope_model)<hope_scale)
          hope_scale = abs(hope_bleu) / abs(hope_model);
        else break;
      }
      // Update weights
      if(hope_index!=fear_index) {
        // Vector difference
        const MiraFeatureVector& hope=train->featuresAt(hope_index);
        const MiraFeatureVector& fear=train->featuresAt(fear_index);
        MiraFeatureVector diff = hope - fear;
        // Bleu difference
        const vector<float>& hope_stats = train->scoresAt(hope_index);
        ValType hopeBleu = sentenceLevelBackgroundBleu(hope_stats, bg);
        const vector<float>& fear_stats = train->scoresAt(fear_index);
        ValType fearBleu = sentenceLevelBackgroundBleu(fear_stats, bg);
        assert(hopeBleu + 1e-8 >= fearBleu);
        ValType delta = hopeBleu - fearBleu;
        // Loss and update
        ValType diff_score = wv.score(diff);
        ValType loss = delta - diff_score;
        if(verbose) {
          cerr << "Updating sent " << train->cur_id() << endl;
          cerr << "Wght: " << wv << endl;
          cerr << "Hope: " << hope << " BLEU:" << hopeBleu << " Score:" << wv.score(hope) << endl;
          cerr << "Fear: " << fear << " BLEU:" << fearBleu << " Score:" << wv.score(fear) << endl;
          cerr << "Diff: " << diff << " BLEU:" << delta << " Score:" << diff_score << endl;
          cerr << "Loss: " << loss << " Scale: " << hope_scale << endl;
          cerr << endl;
        }
        if(loss > 0) {
          ValType eta = min(c, loss / diff.sqrNorm());
          wv.update(diff,eta);
          totalLoss+=loss;
          iNumUpdates++;
        }
        // Update BLEU statistics
        const vector<float>& model_stats = train->scoresAt(model_index);
        for(size_t k=0; k<bg.size(); k++) {
          bg[k]*=decay;
          if(model_bg)
            bg[k]+=model_stats[k];
          else
            bg[k]+=hope_stats[k];
        }
      }
      iNumExamples++;
    }
    // Training Epoch summary
    cerr << iNumUpdates << "/" << iNumExamples << " updates"
         << ", avg loss = " << (totalLoss / iNumExamples);


    // Evaluate current average weights
    AvgWeightVector avg = wv.avg();
    ValType bleu = evaluate(train.get(), avg);
    cerr << ", BLEU = " << bleu << endl;
    if(bleu > bestBleu) {
      size_t num_dense = train->num_dense();
      if(initDenseSize>0 && initDenseSize!=num_dense) {
        cerr << "Error: Initial dense feature count and dense feature count from n-best do not match: "
             << initDenseSize << "!=" << num_dense << endl;
        exit(1);
      }
      // Write to a file
      ostream* out;
      ofstream outFile;
      if (!outputFile.empty() ) {
        outFile.open(outputFile.c_str());
        if (!(outFile)) {
          cerr << "Error: Failed to open " << outputFile << endl;
          exit(1);
        }
        out = &outFile;
      } else {
        out = &cout;
      }
      for(size_t i=0; i<avg.size(); i++) {
        if(i<num_dense)
          *out << "F" << i << " " << avg.weight(i) << endl;
        else {
          if(abs(avg.weight(i))>1e-8)
            *out << SparseVector::decode(i-num_dense) << " " << avg.weight(i) << endl;
        }
      }
      outFile.close();
      bestBleu = bleu;
    }
  }
  cerr << "Best BLEU = " << bestBleu << endl;
}
// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:
