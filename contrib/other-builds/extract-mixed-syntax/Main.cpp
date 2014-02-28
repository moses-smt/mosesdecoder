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
    ("GZOutput", po::value<bool>()->default_value(params.gzOutput), "Compress extract files")
    ("GlueGrammar", po::value<string>()->default_value(params.gluePath), "Output glue grammar to here")
    ("SentenceOffset", po::value<long>()->default_value(params.sentenceOffset), "Starting sentence id. Not used")

    ("SourceSyntax", po::value<bool>()->default_value(params.sourceSyntax), "Source sentence is a parse tree")
    ("TargetSyntax", po::value<bool>()->default_value(params.targetSyntax), "Target sentence is a parse tree")
    ("MixedSyntaxType", po::value<int>()->default_value(params.mixedSyntaxType), "Hieu's Mixed syntax type. 0(default)=no mixed syntax, 1=add [X] only if no syntactic label. 2=add [X] everywhere");

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

  if (vm.count("MaxSpan")) params.maxSpan = vm["MaxSpan"].as<int>();
  if (vm.count("GZOutput")) params.gzOutput = true;
  if (vm.count("GlueGrammar")) params.gluePath = vm["GlueGrammar"].as<string>();
  if (vm.count("SentenceOffset")) params.sentenceOffset = vm["SentenceOffset"].as<long>();

  if (vm.count("SourceSyntax")) params.sourceSyntax = true;
  if (vm.count("TargetSyntax")) params.targetSyntax = true;
  if (vm.count("MixedSyntaxType")) params.mixedSyntaxType = vm["MixedSyntaxType"].as<int>();

  // input files;
  string pathTarget = argv[1];
  string pathSource = argv[2];
  string pathAlignment = argv[3];

  string pathExtract = argv[4];
  string pathExtractInv = pathExtract;
  if (params.gzOutput) {
	  pathExtract += ".gz";
	  pathExtractInv += ".gz";
  }

  Moses::InputFileStream strmTarget(pathTarget);
  Moses::InputFileStream strmSource(pathSource);
  Moses::InputFileStream strmAlignment(pathAlignment);
  Moses::OutputFileStream extractFile(pathExtract);
  Moses::OutputFileStream extractInvFile(pathExtractInv);


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

	  rules.Output(extractFile, true);
	  rules.Output(extractInvFile, false);

	  delete alignedSentence;
  }


  cerr << "Finished" << endl;
}

