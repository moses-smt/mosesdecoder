// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011- University of Edinburgh

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

#include <fstream>
#include <iomanip>
#include <boost/program_options.hpp>
#include "M2.h"

using namespace MosesTuning;

namespace po = boost::program_options;

int main(int argc, char** argv) {
  bool help;

  std::string candidate;
  std::string reference;

  float beta;
  bool ignore_case = false, verbose = false;
  size_t max_unchanged_words;

  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("candidate", po::value<std::string>(&candidate), "Candidate")
  ("reference", po::value<std::string>(&reference), "Reference")
  ("beta", po::value<float>(&beta)->default_value(0.5), "Beta")
  ("max-unchanged-words", po::value<size_t>(&max_unchanged_words)->default_value(2), "Max unchanged words") 
  ("ignore-case", po::value<bool>(&ignore_case)->zero_tokens()->default_value(false), "Ignore case")
  ("verbose", po::value<bool>(&verbose)->zero_tokens()->default_value(false), "Verbose output")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);
  if (help) {
    std::cout << "Usage: " + std::string(argv[0]) +  " [options]" << std::endl;
    std::cout << desc << std::endl;
    exit(0);
  }
  
  M2::M2 m2(max_unchanged_words, beta, !ignore_case, verbose);
  m2.ReadM2(reference);

  std::ifstream cand;
  cand.open(candidate.c_str());
  std::string line; 
  size_t i = 0;
  
  M2::Stats stats(4, 0);
  while(std::getline(cand, line)) {
    m2.SufStats(line, i, stats);
    ++i;
  }

  float p, r, f;
  m2.FScore(stats, p, r, f);
  
  std::cout << std::setprecision(4) << std::fixed;
  std::cout << p << std::endl;
  std::cout << r << std::endl;
  std::cout << f << std::endl;

  return 0;
}
