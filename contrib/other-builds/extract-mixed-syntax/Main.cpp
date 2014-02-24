#include <iostream>
#include <cstdlib>
#include <boost/program_options.hpp>

#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "AlignedSentence.h"
#include "Parameter.h"
#include "Rules.h"

using namespace std;

int main(int argc, char** argv)
{
  cerr << "Starting" << endl;

  Parameter params;

  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("add", "additional options")
    ("revision,r", po::value<int>()->default_value(0), "Revision");

  po::variables_map vm;



  // input files;
  string pathTarget = argv[1];
  string pathSource = argv[2];
  string pathAlignment = argv[3];
  string pathExtract = argv[4];

  Moses::InputFileStream strmTarget(pathTarget);
  Moses::InputFileStream strmSource(pathSource);
  Moses::InputFileStream strmAlignment(pathAlignment);
  Moses::OutputFileStream m_extractFile(pathExtract);


  // MAIN LOOP
  string lineTarget, lineSource, lineAlignment;
  while (getline(strmTarget, lineTarget)) {
	  bool success;
	  success = getline(strmSource, lineSource);
	  if (!success) {
		  throw "Couldn't read source";
	  }
	  success = getline(strmAlignment, lineAlignment);
	  if (!success) {
		  throw "Couldn't read alignment";
	  }

	  cerr << "lineTarget=" << lineTarget << endl;
	  cerr << "lineSource=" << lineSource << endl;
	  cerr << "lineAlignment=" << lineAlignment << endl;

	  AlignedSentence alignedSentence(lineSource, lineTarget, lineAlignment);
	  alignedSentence.CreateConsistentPhrases(params);
	  cerr << alignedSentence.Debug();

	  Rules rules(alignedSentence, params);
	  rules.Extend(params);
	  cerr << rules.Debug();

	  rules.Output(cout);
  }


  cerr << "Finished" << endl;
}

