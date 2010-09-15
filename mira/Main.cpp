/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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

#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "StaticData.h"

#include "Decoder.h"

using namespace Mira;
using namespace std;
using namespace Moses;
namespace po = boost::program_options;

int main(int argc, char** argv) {
  bool help;
  int verbosity;
  string mosesConfigFile;
  string inputFile;
  vector<string> referenceFiles;
  po::options_description desc("Allowed options");
  desc.add_options()
        ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
        ("config,f",po::value<string>(&mosesConfigFile),"Moses ini file")
        ("verbosity,v", po::value<int>(&verbosity)->default_value(0), "Verbosity level")
        ("input-file,i",po::value<string>(&inputFile),"Input file containing tokenised source")
        ("reference-files,r", po::value<vector<string> >(&referenceFiles), "Reference translation files for training");

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
  
  if (mosesConfigFile.empty()) {
    cerr << "Error: No moses ini file specified" << endl;
    return 1;
  }

  if (inputFile.empty()) {
    cerr << "Error: No input file specified" << endl;
    return 1;
  }

  if (!referenceFiles.size()) {
    cerr << "Error: No reference files specified" << endl;
    return 1;
  }


  //initialise moses
  initMoses(mosesConfigFile, verbosity, argc, argv);

  //load input and references 

  //Main loop:
    //pick sentence
    //run decoder
    //run optimiser
    //update moses weights
  


    return 0;
}
