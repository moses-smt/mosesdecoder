/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh
 
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

#include "pcfg_score.h"

#include "options.h"
#include "tree_scorer.h"

#include "pcfg-common/exception.h"
#include "pcfg-common/pcfg.h"
#include "pcfg-common/pcfg_tree.h"
#include "pcfg-common/syntax_tree.h"
#include "pcfg-common/typedef.h"
#include "pcfg-common/xml_tree_parser.h"

#include <boost/program_options.hpp>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Moses {
namespace PCFG {

int PcfgScore::Main(int argc, char *argv[]) {
  // Process command-line options.
  Options options;
  ProcessOptions(argc, argv, options);

  // Open PCFG stream.
  std::ifstream pcfg_stream;
  OpenNamedInputOrDie(options.pcfg_file, pcfg_stream);

  // Read PCFG.
  Pcfg pcfg;
  Vocabulary non_term_vocab;
  pcfg.Read(pcfg_stream, non_term_vocab);

  // Score corpus according to PCFG.
  TreeScorer scorer(pcfg, non_term_vocab);
  XmlTreeParser parser;
  XmlTreeWriter<PcfgTree> writer;
  std::string line;
  std::size_t line_num = 0;
  std::auto_ptr<PcfgTree> tree;
  while (std::getline(std::cin, line)) {
    ++line_num;
    try {
      tree = parser.Parse(line);
    } catch (Exception &e) {
      std::ostringstream msg;
      msg << "line " << line_num << ": " << e.msg();
      Error(msg.str());
    }
    if (!tree.get()) {
      std::ostringstream msg;
      msg << "no tree at line " << line_num;
      Warn(msg.str());
      std::cout << line << std::endl;
      continue;
    }
    if (!scorer.Score(*tree)) {
      std::ostringstream msg;
      msg << "failed to score tree at line " << line_num;
      Warn(msg.str());
      std::cout << line << std::endl;
      continue;
    }
    writer.Write(*tree, std::cout);
  }

  return 0;
}

void PcfgScore::ProcessOptions(int argc, char *argv[], Options &options) const {
  namespace po = boost::program_options;

  std::ostringstream usage_top;
  usage_top << "Usage: " << name() << " PCFG\n\n"
            << "Options";

  // Declare the command line options that are visible to the user.
  po::options_description visible(usage_top.str());
  visible.add_options()
    ("help", "print help message and exit")
  ;

  // Declare the command line options that are hidden from the user
  // (these are used as positional options).
  po::options_description hidden("Hidden options");
  hidden.add_options()
    ("pcfg-file", po::value(&options.pcfg_file), "pcfg file")
  ;

  // Compose the full set of command-line options.
  po::options_description cmd_line_options;
  cmd_line_options.add(visible).add(hidden);

  // Register the positional options.
  po::positional_options_description p;
  p.add("pcfg-file", 1);

  // Process the command-line.
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).style(CommonOptionStyle()).
              options(cmd_line_options).positional(p).run(), vm);
    po::notify(vm);
  } catch (const std::exception &e) {
    std::ostringstream msg;
    msg << e.what() << "\n\n" << visible;
    Error(msg.str());
  }

  if (vm.count("help")) {
    std::cout << visible << std::endl;
    std::exit(0);
  }

  // Check positional options were given.

  if (!vm.count("pcfg-file")) {
    std::ostringstream msg;
    msg << "missing required argument\n\n" << visible << std::endl;
    Error(msg.str());
  }
}

}  // namespace PCFG
}  // namespace Moses
