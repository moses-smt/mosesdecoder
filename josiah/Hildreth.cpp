#include <iostream>
#include <sstream>


#include <boost/program_options.hpp>

#include "FeatureVector.h"
#include "OnlineLearner.h"

namespace po = boost::program_options;
using namespace Josiah;
using namespace std;


int main(int argc, char** argv) {
  vector<float> avec;
  float b;
  float C;
  bool help;
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help",po::value( &help )->zero_tokens()->default_value(false), "Print this help message and exit")
  ("a", po::value<vector<float> >(&avec), "Constraint vector")
  ("b", po::value<float>(&b), "Constraint scalar")
  ("c", po::value<float>(&C)->default_value(0.0f), "slack");
  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);
  
  
  
  if (help) {
    cout << "Usage: " + string(argv[0]) +  " -f mosesini-file [options]" << endl;
    cout << desc << endl;
    return 0;
  }

  FVector a;
  for (size_t i = 0; i < avec.size(); ++i) {
    ostringstream name;
    name << i;
    a[name.str()] = avec[i];
  }
  vector<FVector> as;
  as.push_back(a);
  vector<float> bs;
  bs.push_back(b);

  vector<float> alpha;
  if (C) {
    alpha = hildreth(as,bs,C);
  } else {
    alpha = hildreth(as,bs);
  }
  cout << alpha[0] << endl;
  
}

