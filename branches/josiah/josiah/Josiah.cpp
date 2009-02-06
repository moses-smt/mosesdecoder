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

#include <boost/program_options.hpp>

#include "Decoder.h"
#include "Derivation.h"
#include "Gibbler.h"
#include "GibbsOperator.h"

using namespace std;
using namespace Josiah;
using namespace Moses;
namespace po = boost::program_options;

/**
  * Output probabilities of derivations or translations of a given set of source sentences.
 **/
int main(int argc, char** argv) {
  int iterations;
  int topn;
  int debug;
  string inputfile;
  string mosesini;
  
  po::options_description desc("Allowed options");
  desc.add_options()
        ("help", "Print this help message and exit")
        ("config,f",po::value<string>(&mosesini),"Moses ini file")
        ("iterations,s", po::value<int>(&iterations)->default_value(5), "Number of sampling iterations")
        ("input-file,i",po::value<string>(&inputfile),"Input file containing tokenised source")
        ("dumpmd",po::value<int>(&topn)->default_value(20),"Dump the top n derivations to stdout")
        ("verbosity,v", po::value<int>(&debug)->default_value(0), "Verbosity level");
  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
      std::cout << "Usage: " + string(argv[0]) +  " -f mosesini-file [options]" << std::endl;
      std::cout << desc << std::endl;
      return 0;
  }
  
  if (!vm.count("config")) {
      cerr << "Error: No moses ini file specified" << endl;
      return 1;
  }
  
    //set up moses
  initMoses(mosesini,debug);
  Decoder* decoder = new MosesDecoder();

  
  if (!vm.count("input-file")) {
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
  while (*in) {
    string line;
    getline(*in,line);
    if (line.empty()) continue;

    //configure the sampler
    Sampler sampler;
    DerivationCollector* collector = new DerivationCollector();
    sampler.AddOperator(new MergeSplitOperator());
    sampler.AddOperator(new TranslationSwapOperator());
    //sampler.AddOperator(new FlipOperator());
    sampler.AddCollector(collector);
    sampler.SetIterations(iterations);
    
    TranslationOptionCollection* toc;
    Hypothesis* hypothesis;
    decoder->decode(line,hypothesis,toc);
    sampler.Run(hypothesis,toc);
    
    if (topn > 0) {
      vector<DerivationProbability> derivations;
      collector->getTopN(topn,derivations);
      for (size_t i = 0; i < derivations.size(); ++i) {
        cout  << lineno << " "  << std::setprecision(8) << derivations[i].second << " " << *(derivations[i].first) << endl;
      }
    }
    ++lineno;
  }
  
  delete in;
  delete decoder;
}
