#include <iostream>
#include <cstdlib>
#include <boost/program_options.hpp>

using namespace std;

int main(int argc, char** argv)
{
  cerr << "Starting" << endl;

  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("add", "additional options")
    ("source-language,s", po::value<string>()->required(), "Source Language")
    ("target-language,t", po::value<string>()->required(), "Target Language")
    ("revision,r", po::value<int>()->default_value(0), "Revision");

  po::variables_map vm;

}

