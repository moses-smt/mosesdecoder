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

#include <iostream>
#include <iomanip>
#include <fstream>

#include <boost/program_options.hpp>

#include "Decoder.h"
#include "Derivation.h"
#include "Gibbler.h"
#include "GibbsOperator.h"
#include "SentenceBleu.h"
#include "GainFunction.h"
#include "GibblerExpectedLossTraining.h"

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
  * Output probabilities of derivations or translations of a given set of source sentences.
 **/
int main(int argc, char** argv) {
  int iterations;
  int topn;
  int debug;
  string inputfile;
  string mosesini;
  bool decode;
  bool help;
  bool expected_loss_training;
  vector<string> ref_files;
  po::options_description desc("Allowed options");
  desc.add_options()
        ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
        ("config,f",po::value<string>(&mosesini),"Moses ini file")
        ("iterations,s", po::value<int>(&iterations)->default_value(5), "Number of sampling iterations")
        ("input-file,i",po::value<string>(&inputfile),"Input file containing tokenised source")
        ("nbest,n",po::value<int>(&topn)->default_value(20),"Dump the top n derivations to stdout")
        ("decode,d",po::value( &decode )->zero_tokens()->default_value(false),"Write the most likely derivation to stdout")
        ("verbosity,v", po::value<int>(&debug)->default_value(0), "Verbosity level")
        ("elt,t", po::value(&expected_loss_training)->zero_tokens()->default_value(false), "Train to minimize expected loss")
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

  if (mosesini.empty()) {
      cerr << "Error: No moses ini file specified" << endl;
      return 1;
  }
  
    //set up moses
  initMoses(mosesini,debug);
  Decoder* decoder = new MosesDecoder();

  GainFunctionVector g;
  if (ref_files.size() > 0) LoadReferences(ref_files, &g);
  
  if (inputfile.empty()) {
    VERBOSE(1,"Warning: No input file specified, using toy dataset" << endl);
  }

  istream* in = NULL;
  if (inputfile.size()) {
    in = new ifstream(inputfile.c_str());
    if (! *in) {
      cerr << "Error: Failed to open input file: " + inputfile << endl;
      return 1;
    }
  } else {
    in = new istringstream(string("das parlament will das auf zweierlei weise tun ."));
  }
  
  size_t lineno = 0;
  ScoreComponentCollection gradient;
  while (*in) {
    string line;
    getline(*in,line);
    if (line.empty()) continue;

    //configure the sampler
    Sampler sampler;
    DerivationCollector collector;
    GibblerExpectedLossCollector* c2 = expected_loss_training ? new GibblerExpectedLossCollector(g[lineno]) : NULL;
    MergeSplitOperator mso;
    TranslationSwapOperator tso;
    sampler.AddOperator(&mso);
    sampler.AddOperator(&tso);
    sampler.AddOperator(new FlipOperator());
    if (expected_loss_training)
      sampler.AddCollector(c2);
    else
      sampler.AddCollector(&collector);
    sampler.SetIterations(iterations);
    
    TranslationOptionCollection* toc;
    Hypothesis* hypothesis;
    decoder->decode(line,hypothesis,toc);
    sampler.Run(hypothesis,toc);

    if (expected_loss_training) {
      c2->UpdateGradient(&gradient);
      cerr << "Gradient: " << gradient << endl;
    } else {
      if (topn > 0) {
        vector<DerivationProbability> derivations;
        collector.getTopN(topn,derivations);
        for (size_t i = 0; i < derivations.size(); ++i) {
          Derivation d = *(derivations[i].first);
          cout << d << endl;
        
          cout  << lineno << " "  << std::setprecision(8) << derivations[i].second << " " << *(derivations[i].first) << endl;
        }
      }
    }
    delete c2;
    ++lineno;
  }
  
  delete in;
  delete decoder;
}
