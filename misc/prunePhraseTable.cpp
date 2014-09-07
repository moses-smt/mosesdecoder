// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2014- University of Edinburgh

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
  Prune the phrase table using the same translation pruning that Moses uses during decoding.
**/

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "moses/Parameter.h"
#include "moses/StaticData.h"

#include "util/file_piece.hh"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/double-conversion/double-conversion.h"
#include "util/exception.hh"


using namespace Moses;
using namespace std;

namespace po = boost::program_options;

static void usage(const po::options_description& desc, char** argv) {
    cerr << "Usage: " + string(argv[0]) +  " [options] input-file output-file" << endl;
    cerr << desc << endl;
}

//Find top n translations of source, and send them to output
static void outputTopN(const StringPiece& sourcePhrase, const StaticData& staticData, ostream& out) {

}

int main(int argc, char** argv) 
{
  bool help;
  string input_file;
  string output_file;
  string config_file;


  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("input-file,i", po::value<string>(&input_file), "Input file")
  ("output-file,o", po::value<string>(&output_file), "Output file")
  ("config-file,f", po::value<string>(&config_file), "Config file")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);
  if (help) {
    usage(desc, argv);
    exit(0);
  }
  if (input_file.empty()) {
    cerr << "ERROR: Please specify an input file" << endl << endl;
    usage(desc, argv);
    exit(1);
  }
  if (output_file.empty()) {
    cerr << "ERROR: Please specify an output file" << endl << endl;
    usage(desc, argv);
    exit(1);
  }
  if (config_file.empty()) {
    cerr << "ERROR: Please specify a config file" << endl << endl;
    usage(desc, argv);
    exit(1);
  }

  vector<string> mosesargs;
  mosesargs.push_back(argv[0]);
  mosesargs.push_back("-f");
  mosesargs.push_back(config_file);

  boost::scoped_ptr<Parameter> params(new Parameter());  
  char** mosesargv = new char*[mosesargs.size()];
  for (size_t i = 0; i < mosesargs.size(); ++i) {
    mosesargv[i] = new char[mosesargs[i].length() + 1];
    strcpy(mosesargv[i], mosesargs[i].c_str());
  }

  if (!params->LoadParam(mosesargs.size(), mosesargv)) {
    params->Explain();
    exit(1);
  }

  if (!StaticData::LoadDataStatic(params.get(),argv[0])) {
    exit(1);
  }


  //
  //Load and prune the phrase table. This is taken (with mods) from moses/TranslationModel/RuleTable/LoaderStandard.cpp
  //
  const StaticData &staticData = StaticData::Instance();

  string lineOrig;

  std::ostream *progress = NULL;
  IFVERBOSE(1) progress = &std::cerr;
  util::FilePiece in(input_file.c_str(), progress);

  // reused variables
  vector<float> scoreVector;
  StringPiece line;

  double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");

  StringPiece previous;

  while(true) {
    try {
      line = in.ReadLine();
    } catch (const util::EndOfFileException &e) {
      break;
    }

    util::TokenIter<util::MultiCharacter> pipes(line, "|||");
    StringPiece sourcePhraseString(*pipes);
    if (sourcePhraseString != previous) {
      outputTopN(previous, staticData, cout);
    }
  }





  return 0;
}
