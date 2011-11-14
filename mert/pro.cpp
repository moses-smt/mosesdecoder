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


/** 
  * This is part of the PRO implementation. It converts the features and scores 
  * files into a form suitable for input into the megam maxent trainer.
  *
  *   For details of PRO, refer to Hopkins & May (EMNLP 2011)
 **/
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "FeatureDataIterator.h"

using namespace std;

namespace po = boost::program_options;

int main(int argc, char** argv) 
{
  bool help;
  vector<string> scoreFiles;
  vector<string> featureFiles;
  int seed;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
    ("scfile,S", po::value<vector<string> >(&scoreFiles), "Scorer data files")
    ("ffile,F", po::value<vector<string> > (&featureFiles), "Feature data files")
    ("random-seed,r", po::value<int>(&seed), "Seed for random number generation")
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
      return 0;
  }
  
  if (vm.count("random-seed")) {
    cerr << "Initialising random seed to " << seed << endl;
    srand(seed);
  } else {
    cerr << "Initialising random seed from system clock" << endl;
    srand(time(NULL));
  }

  FeatureDataIterator fi(featureFiles[0]);
  for (; fi != FeatureDataIterator::end(); ++fi) {
    const vector<FeatureDataItem>& featureData = *fi;
  }

}

