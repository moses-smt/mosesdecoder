/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2015- University of Edinburgh

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

/**
 * Used to test that hypergraph decoding works correctly.
**/
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "HopeFearDecoder.h"

using namespace std;
using namespace MosesTuning;

namespace po = boost::program_options;


int main(int argc, char** argv)
{
  bool help;
  string denseInitFile;
  string sparseInitFile;
  string hypergraphFile;
  size_t edgeCount = 500;

  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("dense-init,d", po::value<string>(&denseInitFile), "Weight file for dense features.")
  ("sparse-init,s", po::value<string>(&sparseInitFile), "Weight file for sparse features")
  ("hypergraph,g", po::value<string>(&hypergraphFile), "File containing compressed hypergraph")
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

  if (hypergraphFile.empty()) {
    cerr << "Error: missing hypergraph file" << endl;
    exit(1);
  }

  Vocab vocab;

  //Add dummy reference
  ReferenceSet references;
  references.AddLine(0,"blah blah blah", vocab);

  //Load weights
  pair<MiraWeightVector*, size_t> ret = InitialiseWeights(denseInitFile, sparseInitFile, "hypergraph", true);
  boost::scoped_ptr<MiraWeightVector> wv(ret.first);
  size_t initDenseSize = ret.second;
  SparseVector weights;
  wv->ToSparse(&weights, initDenseSize);

  //Load hypergraph
  Graph graph(vocab);
  util::scoped_fd fd(util::OpenReadOrThrow(hypergraphFile.c_str()));
  util::FilePiece file(fd.release());
  ReadGraph(file,graph);

  boost::shared_ptr<Graph> prunedGraph;
  prunedGraph.reset(new Graph(vocab));
  graph.Prune(prunedGraph.get(), weights, edgeCount);

  vector<ValType> bg(9);
  HgHypothesis bestHypo;
  //best hypothesis
  Viterbi(*prunedGraph, weights, 0, references, 0, bg, &bestHypo);

  for (size_t i = 0; i < bestHypo.text.size(); ++i) {
    cout << bestHypo.text[i]->first << " ";
  }
  cout << endl;

  //write weights
  cerr << "WEIGHTS ";
  bestHypo.featureVector.write(cerr, "=");
  cerr << endl;

}

