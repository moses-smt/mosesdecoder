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

#include "util/exception.hh"
#include "util/random.hh"

#include "BleuScorer.h"
#include "CHRFScorer.h"
#include "HopeFearDecoder.h"
#include "MiraFeatureVector.h"
#include "MiraWeightVector.h"

#include "Scorer.h"
#include "ScorerFactory.h"

using namespace std;
using namespace MosesTuning;

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  bool help;
  string denseInitFile;
  string sparseInitFile;
  string type = "nbest";
  string sctype = "BLEU";
  string scconfig = "";
  vector<string> scoreFiles;
  vector<string> featureFiles;
  vector<string> referenceFiles; //for hg mira
  string hgDir;
  int seed;
  string outputFile;
  float c = 0.01;      // Step-size cap C
  float decay = 0.999; // Pseudo-corpus decay \gamma
  int n_iters = 60;    // Max epochs J
  bool streaming = false; // Stream all k-best lists?
  bool streaming_out = false; // Stream output after each sentence?
  bool no_shuffle = false; // Don't shuffle, even for in memory version
  bool model_bg = false; // Use model for background corpus
  bool verbose = false; // Verbose updates
  bool safe_hope = false; // Model score cannot have more than BLEU_RATIO times more influence than BLEU
  size_t hgPruning = 50; //prune hypergraphs to have this many edges per reference word

  // Command-line processing follows pro.cpp
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("type,t", po::value<string>(&type), "Either nbest or hypergraph")
  ("sctype", po::value<string>(&sctype), "the scorer type (default BLEU)")
  ("scconfig,c", po::value<string>(&scconfig), "configuration string passed to scorer")
  ("scfile,S", po::value<vector<string> >(&scoreFiles), "Scorer data files")
  ("ffile,F", po::value<vector<string> > (&featureFiles), "Feature data files")
  ("hgdir,H", po::value<string> (&hgDir), "Directory containing hypergraphs")
  ("reference,R", po::value<vector<string> > (&referenceFiles), "Reference files, only required for hypergraph mira")
  ("random-seed,r", po::value<int>(&seed), "Seed for random number generation")
  ("output-file,o", po::value<string>(&outputFile), "Output file")
  ("cparam,C", po::value<float>(&c), "MIRA C-parameter, lower for more regularization (default 0.01)")
  ("decay,D", po::value<float>(&decay), "BLEU background corpus decay rate (default 0.999)")
  ("iters,J", po::value<int>(&n_iters), "Number of MIRA iterations to run (default 60)")
  ("dense-init,d", po::value<string>(&denseInitFile), "Weight file for dense features. This should have 'name= value' on each line, or (legacy) should be the Moses mert 'init.opt' format.")
  ("sparse-init,s", po::value<string>(&sparseInitFile), "Weight file for sparse features")
  ("streaming", po::value(&streaming)->zero_tokens()->default_value(false), "Stream n-best lists to save memory, implies --no-shuffle")
  ("streaming-out", po::value(&streaming_out)->zero_tokens()->default_value(false), "Stream weights to stdout after each sentence")
  ("no-shuffle", po::value(&no_shuffle)->zero_tokens()->default_value(false), "Don't shuffle hypotheses before each epoch")
  ("model-bg", po::value(&model_bg)->zero_tokens()->default_value(false), "Use model instead of hope for BLEU background")
  ("verbose", po::value(&verbose)->zero_tokens()->default_value(false), "Verbose updates")
  ("safe-hope", po::value(&safe_hope)->zero_tokens()->default_value(false), "Mode score's influence on hope decoding is limited")
  ("hg-prune", po::value<size_t>(&hgPruning), "Prune hypergraphs to have this many edges per reference word")
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
    util::rand_init(seed);
  } else {
    cerr << "Initialising random seed from system clock" << endl;
    util::rand_init();
  }


  pair<MiraWeightVector*, size_t> ret = InitialiseWeights(denseInitFile, sparseInitFile, type, verbose);
  boost::scoped_ptr<MiraWeightVector> wv(ret.first);
  size_t initDenseSize = ret.second;

  // Initialize scorer
  if(sctype != "BLEU" && type == "hypergraph") {
    UTIL_THROW(util::Exception, "hypergraph mira only supports BLEU");
  }
  boost::scoped_ptr<Scorer> scorer(ScorerFactory::getScorer(sctype, scconfig));

  // Initialize background corpus
  vector<ValType> bg(scorer->NumberOfScores(), 1);

  boost::scoped_ptr<HopeFearDecoder> decoder;
  if (type == "nbest") {
    decoder.reset(new NbestHopeFearDecoder(featureFiles, scoreFiles, streaming, no_shuffle, safe_hope, scorer.get()));
  } else if (type == "hypergraph") {
    decoder.reset(new HypergraphHopeFearDecoder(hgDir, referenceFiles, initDenseSize, streaming, no_shuffle, safe_hope, hgPruning, *wv, scorer.get()));
  } else {
    UTIL_THROW(util::Exception, "Unknown batch mira type: '" << type << "'");
  }

  // Training loop
  if (!streaming_out)
    cerr << "Initial BLEU = " << decoder->Evaluate(wv->avg()) << endl;
  ValType bestBleu = 0;
  for(int j=0; j<n_iters; j++) {
    // MIRA train for one epoch
    int iNumExamples = 0;
    int iNumUpdates = 0;
    ValType totalLoss = 0.0;
    size_t sentenceIndex = 0;
    for(decoder->reset(); !decoder->finished(); decoder->next()) {
      HopeFearData hfd;
      decoder->HopeFear(bg,*wv,&hfd);

      // Update weights
      if (!hfd.hopeFearEqual && hfd.hopeBleu  > hfd.fearBleu) {
        // Vector difference
        MiraFeatureVector diff = hfd.hopeFeatures - hfd.fearFeatures;
        // Bleu difference
        //assert(hfd.hopeBleu + 1e-8 >= hfd.fearBleu);
        ValType delta = hfd.hopeBleu - hfd.fearBleu;
        // Loss and update
        ValType diff_score = wv->score(diff);
        ValType loss = delta - diff_score;
        if(verbose) {
          cerr << "Updating sent " << sentenceIndex << endl;
          cerr << "Wght: " << *wv << endl;
          cerr << "Hope: " << hfd.hopeFeatures << " BLEU:" << hfd.hopeBleu << " Score:" << wv->score(hfd.hopeFeatures) << endl;
          cerr << "Fear: " << hfd.fearFeatures << " BLEU:" << hfd.fearBleu << " Score:" << wv->score(hfd.fearFeatures) << endl;
          cerr << "Diff: " << diff << " BLEU:" << delta << " Score:" << diff_score << endl;
          cerr << "Loss: " << loss <<  " Scale: " << 1 << endl;
          cerr << endl;
        }
        if(loss > 0) {
          ValType eta = min(c, loss / diff.sqrNorm());
          wv->update(diff,eta);
          totalLoss+=loss;
          iNumUpdates++;
        }
        // Update BLEU statistics
        for(size_t k=0; k<bg.size(); k++) {
          bg[k]*=decay;
          if(model_bg)
            bg[k]+=hfd.modelStats[k];
          else
            bg[k]+=hfd.hopeStats[k];
        }
      }
      iNumExamples++;
      ++sentenceIndex;
      if (streaming_out)
        cout << *wv << endl;
    }
    // Training Epoch summary
    cerr << iNumUpdates << "/" << iNumExamples << " updates"
         << ", avg loss = " << (totalLoss / iNumExamples);


    // Evaluate current average weights
    AvgWeightVector avg = wv->avg();
    ValType bleu = decoder->Evaluate(avg);
    cerr << ", BLEU = " << bleu << endl;
    if(bleu > bestBleu) {
      /*
      size_t num_dense = train->num_dense();
      if(initDenseSize>0 && initDenseSize!=num_dense) {
        cerr << "Error: Initial dense feature count and dense feature count from n-best do not match: "
             << initDenseSize << "!=" << num_dense << endl;
        exit(1);
      }*/
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
        if(i<initDenseSize)
          *out << "F" << i << " " << avg.weight(i) << endl;
        else {
          if(abs(avg.weight(i))>1e-8)
            *out << SparseVector::decode(i-initDenseSize) << " " << avg.weight(i) << endl;
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
