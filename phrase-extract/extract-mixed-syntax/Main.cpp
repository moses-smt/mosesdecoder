#include <iostream>
#include <cstdlib>
#include <boost/program_options.hpp>

#include "Main.h"
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
  ("MinSpan", po::value<int>()->default_value(params.minSpan), "Min (source) span of a rule.")
  ("GlueGrammar", po::value<string>()->default_value(params.gluePath), "Output glue grammar to here")
  ("SentenceOffset", po::value<long>()->default_value(params.sentenceOffset), "Starting sentence id. Not used")
  ("GZOutput", "Compress extract files")
  ("MaxNonTerm", po::value<int>()->default_value(params.maxNonTerm), "Maximum number of non-terms allowed per rule")
  ("MaxHieroNonTerm", po::value<int>()->default_value(params.maxHieroNonTerm), "Maximum number of Hiero non-term. Usually, --MaxNonTerm is the normal constraint")
  ("MinHoleSource", po::value<int>()->default_value(params.minHoleSource), "Minimum source span for a non-term.")
  ("MinHoleSourceSyntax", po::value<int>()->default_value(params.minHoleSourceSyntax), "Minimum source span for a syntactic non-term (source or target).")

  ("SourceSyntax", "Source sentence is a parse tree")
  ("TargetSyntax", "Target sentence is a parse tree")
  ("MixedSyntaxType", po::value<int>()->default_value(params.mixedSyntaxType), "Hieu's Mixed syntax type. 0(default)=no mixed syntax, 1=add [X] only if no syntactic label. 2=add [X] everywhere")
  ("MultiLabel", po::value<int>()->default_value(params.multiLabel), "What to do with multiple labels on the same span. 0(default)=keep them all, 1=keep only top-most, 2=keep only bottom-most")
  ("HieroSourceLHS", "Always use Hiero source LHS? Default = 0")
  ("MaxSpanFreeNonTermSource", po::value<int>()->default_value(params.maxSpanFreeNonTermSource), "Max number of words covered by beginning/end NT. Default = 0 (no limit)")
  ("NoNieceTerminal", "Don't extract rule if 1 of the non-term covers the same word as 1 of the terminals")
  ("MaxScope", po::value<int>()->default_value(params.maxScope), "maximum scope (see Hopkins and Langmead (2010)). Default is HIGH")
  ("MinScope", po::value<int>()->default_value(params.minScope), "min scope.")

  ("SpanLength", "Property - span length of each LHS non-term")
  ("RuleLength", "Property - length of entire rule. Only for rules with NTs")

  ("NonTermContext", "Property - (source) left and right, inside and outside words of each non-term ")
  ("NonTermContextTarget", "Property - (target) left and right, inside and outside words of each non-term")
  ("NonTermContextFactor", po::value<int>()->default_value(params.nonTermContextFactor), "Factor to use for non-term context property.")

  ("NumSourceFactors", po::value<int>()->default_value(params.numSourceFactors), "Number of source factors.")
  ("NumTargetFactors", po::value<int>()->default_value(params.numTargetFactors), "Number of target factors.")

  ("HieroNonTerm", po::value<string>()->default_value(params.hieroNonTerm), "Hiero non-terminal label, including bracket")
  ("ScopeSpan", po::value<string>()->default_value(params.scopeSpanStr), "Min and max span for rules of each scope. Format is min,max:min,max...")

  ("NonTermConsecSource", "Allow consecutive non-terms on the source side")
  ("NonTermConsecSourceMixedSyntax", po::value<int>()->default_value(params.nonTermConsecSourceMixedSyntax), "In mixed syntax mode, what nt can be consecutive. 0=don't allow consec nt. 1(default)=hiero+syntax. 2=syntax+syntax. 3=always allow");


  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc),
              vm); // can throw

    /** --help option
     */
    if ( vm.count("help") || argc < 5 ) {
      std::cout << argv[0] << " target source alignment [options...]" << std::endl
                << desc << std::endl;
      return EXIT_SUCCESS;
    }

    po::notify(vm); // throws on error, so do after help in case
    // there are any problems
  } catch(po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
    std::cerr << desc << std::endl;
    return EXIT_FAILURE;
  }

  if (vm.count("MaxSpan")) params.maxSpan = vm["MaxSpan"].as<int>();
  if (vm.count("MinSpan")) params.minSpan = vm["MinSpan"].as<int>();
  if (vm.count("GZOutput")) params.gzOutput = true;
  if (vm.count("GlueGrammar")) params.gluePath = vm["GlueGrammar"].as<string>();
  if (vm.count("SentenceOffset")) params.sentenceOffset = vm["SentenceOffset"].as<long>();
  if (vm.count("MaxNonTerm")) params.maxNonTerm = vm["MaxNonTerm"].as<int>();
  if (vm.count("MaxHieroNonTerm")) params.maxHieroNonTerm = vm["MaxHieroNonTerm"].as<int>();
  if (vm.count("MinHoleSource")) params.minHoleSource = vm["MinHoleSource"].as<int>();
  if (vm.count("MinHoleSourceSyntax")) params.minHoleSourceSyntax = vm["MinHoleSourceSyntax"].as<int>();

  if (vm.count("SourceSyntax")) params.sourceSyntax = true;
  if (vm.count("TargetSyntax")) params.targetSyntax = true;
  if (vm.count("MixedSyntaxType")) params.mixedSyntaxType = vm["MixedSyntaxType"].as<int>();
  if (vm.count("MultiLabel")) params.multiLabel = vm["MultiLabel"].as<int>();
  if (vm.count("HieroSourceLHS")) params.hieroSourceLHS = true;
  if (vm.count("MaxSpanFreeNonTermSource")) params.maxSpanFreeNonTermSource = vm["MaxSpanFreeNonTermSource"].as<int>();
  if (vm.count("NoNieceTerminal")) params.nieceTerminal = false;
  if (vm.count("MaxScope")) params.maxScope = vm["MaxScope"].as<int>();
  if (vm.count("MinScope")) params.minScope = vm["MinScope"].as<int>();

  // properties
  if (vm.count("SpanLength")) params.spanLength = true;
  if (vm.count("RuleLength")) params.ruleLength = true;
  if (vm.count("NonTermContext")) params.nonTermContext = true;
  if (vm.count("NonTermContextTarget")) params.nonTermContextTarget = true;
  if (vm.count("NonTermContextFactor")) params.nonTermContextFactor = vm["NonTermContextFactor"].as<int>();

  if (vm.count("NumSourceFactors")) params.numSourceFactors = vm["NumSourceFactors"].as<int>();
  if (vm.count("NumTargetFactors")) params.numTargetFactors = vm["NumTargetFactors"].as<int>();

  if (vm.count("HieroNonTerm")) params.hieroNonTerm = vm["HieroNonTerm"].as<string>();
  if (vm.count("ScopeSpan")) {
    params.SetScopeSpan(vm["ScopeSpan"].as<string>());
  }

  if (vm.count("NonTermConsecSource")) params.nonTermConsecSource = true;
  if (vm.count("NonTermConsecSourceMixedSyntax")) params.nonTermConsecSourceMixedSyntax = vm["NonTermConsecSourceMixedSyntax"].as<int>();


  // input files;
  string pathTarget = argv[1];
  string pathSource = argv[2];
  string pathAlignment = argv[3];

  string pathExtract = argv[4];
  string pathExtractInv = pathExtract + ".inv";
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
  int lineNum = 1;
  string lineTarget, lineSource, lineAlignment;
  while (getline(strmTarget, lineTarget)) {
    if (lineNum % 10000 == 0) {
      cerr << lineNum << " ";
    }

    if (!getline(strmSource, lineSource)) {
      throw "Couldn't read source";
    }
    if (!getline(strmAlignment, lineAlignment)) {
      throw "Couldn't read alignment";
    }

    /*
    cerr << "lineTarget=" << lineTarget << endl;
    cerr << "lineSource=" << lineSource << endl;
    cerr << "lineAlignment=" << lineAlignment << endl;
    */

    AlignedSentence *alignedSentence;

    if (params.sourceSyntax || params.targetSyntax) {
      alignedSentence = new AlignedSentenceSyntax(lineNum, lineSource, lineTarget, lineAlignment);
    } else {
      alignedSentence = new AlignedSentence(lineNum, lineSource, lineTarget, lineAlignment);
    }

    alignedSentence->Create(params);
    //cerr << alignedSentence->Debug();

    Rules rules(*alignedSentence);
    rules.Extend(params);
    rules.Consolidate(params);
    //cerr << rules.Debug();

    rules.Output(extractFile, true, params);
    rules.Output(extractInvFile, false, params);

    delete alignedSentence;

    ++lineNum;
  }

  if (!params.gluePath.empty()) {
    Moses::OutputFileStream glueFile(params.gluePath);
    CreateGlueGrammar(glueFile);
  }

  cerr << "Finished" << endl;
}

void CreateGlueGrammar(Moses::OutputFileStream &glueFile)
{
  glueFile << "<s> [X] ||| <s> [S] ||| 1 ||| ||| 0" << endl
           << "[X][S] </s> [X] ||| [X][S] </s> [S] ||| 1 ||| 0-0 ||| 0" << endl
           << "[X][S] [X][X] [X] ||| [X][S] [X][X] [S] ||| 2.718 ||| 0-0 1-1 ||| 0" << endl;

}
