/******************************************************************************
 IrstLM: IRST Language Model Toolkit, compile LM
 Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

******************************************************************************/


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>

#include "lmtable.h"


/* GLOBAL OPTIONS ***************/
std::string sn = "0";
std::string sres = "0";
std::string sdecay = "0.95";
/********************************/

void usage(const char *msg = 0) {
  if (msg) { std::cerr << msg << std::endl; }
  std::cerr << "Usage: compile-lm [options] input-file.lm [output-file.blm]" << std::endl;
  if (!msg) std::cerr << std::endl
            << "  compile-lm reads a standard LM file in ARPA format and produces" << std::endl
            << "  a compiled representation that the IRST LM toolkit can quickly" << std::endl
            << "  read and process." << std::endl << std::endl;
  std::cerr << "Options:\n  -r=RESOLUTION\n  -d=DECAY\n  -n=NGRAM SIZE <required>\n\n";
}

bool starts_with(const std::string &s, const std::string &pre) {
  if (pre.size() > s.size()) return false;

  if (pre == s) return true;
  std::string pre_equals(pre+'=');
  if (pre_equals.size() > s.size()) return false;
  return (s.substr(0,pre_equals.size()) == pre_equals);
}

std::string get_param(const std::string& opt, int argc, const char **argv, int& argi)
{
  std::string::size_type equals = opt.find_first_of('=');
  if (equals != std::string::npos && equals < opt.size()-1) {
    return opt.substr(equals+1);
  }
  std::string nexto;
  if (argi + 1 < argc) { 
    nexto = argv[++argi]; 
  } else {
    usage((opt + " requires a value!").c_str());
    exit(1);
  }
  return nexto;
}

void handle_option(const std::string& opt, int argc, const char **argv, int& argi)
{
  if (opt == "--help" || opt == "-h") { usage(); exit(1); }
  if (starts_with(opt, "--resolution") || starts_with(opt, "-r"))
    sres = get_param(opt, argc, argv, argi);
  else if (starts_with(opt, "--decay") || starts_with(opt, "-d"))
    sdecay = get_param(opt, argc, argv, argi);
  else if (starts_with(opt, "--ngram-size") || starts_with(opt, "-n"))
    sn = get_param(opt, argc, argv, argi);
  else {
    usage(("Don't understand option " + opt).c_str());
    exit(1);
  }
}

int main(int argc, const char **argv)
{
  if (argc < 2) { usage(); exit(1); }
  std::vector<std::string> files;
  for (int i=1; i < argc; i++) {
    std::string opt = argv[i];
    if (opt[0] == '-') { handle_option(opt, argc, argv, i); }
      else files.push_back(opt);
  }
  if (files.size() > 2) { usage("Too many arguments"); exit(1); }
  if (files.size() < 1) { usage("Please specify a LM file to read from"); exit(1); }
  double decay = strtod(sdecay.c_str(),0);
  int resolution = strtol(sres.c_str(),0,10);
  int ngram_size = strtol(sn.c_str(),0,10);
  if (ngram_size < 1) { usage("Please specify an ngram size greater than or equal 1 with -n"); exit(1); }
  std::string infile = files[0];
  if (files.size() == 1) {
    std::string::size_type p = infile.rfind('/');
    if (p != std::string::npos && ((p+1) < infile.size())) {
      files.push_back(infile.substr(p+1) + ".blm");
    } else {
      files.push_back(infile + ".blm");
    }
  }
  std::string outfile = files[1];
  std::cout << "Using decay=" << decay << ", resolution=" << resolution << std::endl;
  std::cout << "Reading " << infile << "..." << std::endl;
  std::ifstream inp(infile.c_str());
  if (!inp.good()) {
    std::cerr << "Failed to open " << infile << "!\n";
    exit(1);
  }
  lmtable lmt(inp, ngram_size, resolution, decay);
  std::cout << "Saving to " << outfile << std::endl;
  lmt.savebin(outfile.c_str());
  return 0;
}

