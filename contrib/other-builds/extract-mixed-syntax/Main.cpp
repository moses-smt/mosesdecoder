#include <iostream>
#include <cstdlib>
#include <boost/program_options.hpp>

#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "AlignedSentence.h"
#include "AlignedSentenceSyntax.h"
#include "Parameter.h"
#include "Rules.h"

using namespace std;

bool g_debug = false;

int main(int argc, char** argv)
{
  cerr << "Starting" << endl;

  Parameter params;

  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("MaxSpan", po::value<int>()->default_value(params.maxSpan), "Max (source) span of a rule. ie. number of words in the source")
    ("SourceSyntax", "Source sentence is a parse tree")
    ("TargetSyntax", "Target sentence is a parse tree");

  po::variables_map vm;
  try
  {
    po::store(po::parse_command_line(argc, argv, desc),
              vm); // can throw

    /** --help option
     */
    if ( vm.count("help") || argc < 5 )
    {
      std::cout << "Basic Command Line Parameter App" << std::endl
                << desc << std::endl;
      return EXIT_SUCCESS;
    }

    po::notify(vm); // throws on error, so do after help in case
                    // there are any problems
  }
  catch(po::error& e)
  {
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
    std::cerr << desc << std::endl;
    return EXIT_FAILURE;
  }

  if (vm.count("maxSpan")) params.maxSpan = vm["maxSpan"].as<int>();
  if (vm.count("SourceSyntax")) params.sourceSyntax = true;
  if (vm.count("TargetSyntax")) params.targetSyntax = true;

  // input files;
  string pathTarget = argv[1];
  string pathSource = argv[2];
  string pathAlignment = argv[3];
  string pathExtract = argv[4];

  Moses::InputFileStream strmTarget(pathTarget);
  Moses::InputFileStream strmSource(pathSource);
  Moses::InputFileStream strmAlignment(pathAlignment);
  Moses::OutputFileStream m_extractFile(pathExtract);
  Moses::OutputFileStream m_extractInvFile(pathExtract + ".inv");


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

	  /*
	  cerr << "lineTarget=" << lineTarget << endl;
	  cerr << "lineSource=" << lineSource << endl;
	  cerr << "lineAlignment=" << lineAlignment << endl;
	   */

	  AlignedSentence *alignedSentence;

	  if (params.sourceSyntax || params.targetSyntax) {
		  alignedSentence = new AlignedSentenceSyntax(lineSource, lineTarget, lineAlignment);
	  }
	  else {
		  alignedSentence = new AlignedSentence(lineSource, lineTarget, lineAlignment);
	  }

	  alignedSentence->Create(params);
	  //cerr << alignedSentence->Debug();

	  Rules rules(*alignedSentence);
	  rules.Extend(params);
	  rules.Consolidate(params);
	  //cerr << rules.Debug();

	  rules.Output(m_extractFile, true);
	  rules.Output(m_extractInvFile, false);

	  delete alignedSentence;
  }


  cerr << "Finished" << endl;
}

