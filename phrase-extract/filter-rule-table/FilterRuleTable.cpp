#include "FilterRuleTable.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>

#include "syntax-common/exception.h"
#include "syntax-common/xml_tree_parser.h"

#include "InputFileStream.h"

#include "ForestTsgFilter.h"
#include "Options.h"
#include "StringCfgFilter.h"
#include "StringForest.h"
#include "StringForestParser.h"
#include "TreeCfgFilter.h"
#include "TreeTsgFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

int FilterRuleTable::Main(int argc, char *argv[])
{
  enum TestSentenceFormat {
    kUnknownTestSentenceFormat,
    kString,
    kTree,
    kForest
  };

  enum SourceSideRuleFormat {
    kUnknownSourceSideRuleFormat,
    kCfg,
    kTsg
  };

  // Process command-line options.
  Options options;
  ProcessOptions(argc, argv, options);

  // Open input file.
  Moses::InputFileStream testStream(options.testSetFile);

  // Determine the expected test sentence format and source-side rule format
  // based on the argument to the options.model parameter.
  TestSentenceFormat testSentenceFormat = kUnknownTestSentenceFormat;
  SourceSideRuleFormat sourceSideRuleFormat = kUnknownSourceSideRuleFormat;
  if (options.model == "hierarchical" || options.model == "s2t") {
    testSentenceFormat = kString;
    sourceSideRuleFormat = kCfg;
  } else if (options.model == "t2s") {
    testSentenceFormat = kTree;
    sourceSideRuleFormat = kTsg;
  } else if (options.model == "t2s-scfg") {
    testSentenceFormat = kTree;
    sourceSideRuleFormat = kCfg;
  } else if (options.model == "f2s") {
    testSentenceFormat = kForest;
    sourceSideRuleFormat = kTsg;
  } else {
    Error(std::string("unsupported model type: ") + options.model);
  }

  // Read the test sentences then set up and run the filter.
  if (testSentenceFormat == kString) {
    assert(sourceSideRuleFormat == kCfg);
    std::vector<boost::shared_ptr<std::string> > testStrings;
    ReadTestSet(testStream, testStrings);
    StringCfgFilter filter(testStrings);
    filter.Filter(std::cin, std::cout);
  } else if (testSentenceFormat == kTree) {
    std::vector<boost::shared_ptr<SyntaxTree> > testTrees;
    ReadTestSet(testStream, testTrees);
    if (sourceSideRuleFormat == kCfg) {
      // TODO Implement TreeCfgFilter
      Warn("tree/cfg filtering algorithm not implemented: input will be copied unchanged to output");
      TreeCfgFilter filter(testTrees);
      filter.Filter(std::cin, std::cout);
    } else if (sourceSideRuleFormat == kTsg) {
      TreeTsgFilter filter(testTrees);
      filter.Filter(std::cin, std::cout);
    } else {
      assert(false);
    }
  } else if (testSentenceFormat == kForest) {
    std::vector<boost::shared_ptr<StringForest> > testForests;
    ReadTestSet(testStream, testForests);
    assert(sourceSideRuleFormat == kTsg);
    ForestTsgFilter filter(testForests);
    filter.Filter(std::cin, std::cout);
  }

  return 0;
}

void FilterRuleTable::ReadTestSet(
  std::istream &input,
  std::vector<boost::shared_ptr<std::string> > &sentences)
{
  int lineNum = 0;
  std::string line;
  while (std::getline(input, line)) {
    ++lineNum;
    if (line.empty()) {
      std::cerr << "skipping blank test sentence at line " << lineNum
                << std::endl;
      continue;
    }
    sentences.push_back(boost::make_shared<std::string>(line));
  }
}

void FilterRuleTable::ReadTestSet(
  std::istream &input, std::vector<boost::shared_ptr<SyntaxTree> > &sentences)
{
  XmlTreeParser parser;
  int lineNum = 0;
  std::string line;
  while (std::getline(input, line)) {
    ++lineNum;
    if (line.empty()) {
      std::cerr << "skipping blank test sentence at line " << lineNum
                << std::endl;
      continue;
    }
    sentences.push_back(
      boost::shared_ptr<SyntaxTree>(parser.Parse(line).release()));
  }
}

void FilterRuleTable::ReadTestSet(
  std::istream &input,
  std::vector<boost::shared_ptr<StringForest> > &sentences)
{
  StringForestParser end;
  int sentNum = 0;
  for (StringForestParser p(input); p != end; ++p) {
    ++sentNum;
    if (p->forest->vertices.empty()) {
      std::cerr << "skipping sentence " << sentNum << ": forest is empty"
                << std::endl;
      continue;
    }
    sentences.push_back(p->forest);
  }
}

void FilterRuleTable::ProcessOptions(int argc, char *argv[],
                                     Options &options) const
{
  namespace po = boost::program_options;
  namespace cls = boost::program_options::command_line_style;

  // Construct the 'top' of the usage message: the bit that comes before the
  // options list.
  std::ostringstream usageTop;
  usageTop << "Usage: " << name()
           << " [OPTION]... MODEL TEST\n\n"
           << "Filter for SCFG/STSG rule tables.\n\n"
           << "Options";

  // Construct the 'bottom' of the usage message.
  std::ostringstream usageBottom;
  usageBottom << "\nGiven a rule table on standard input and a set of test sentences, filters out\nthe rules that cannot be applied to any of the test sentences and writes the\nfiltered table to standard output.  MODEL specifies the type of syntax model.\nThe following values are supported:\n\n"
              << "  hierarchical, s2t, t2s, t2s-scfg, f2s\n";

  // Declare the command line options that are visible to the user.
  po::options_description visible(usageTop.str());

  // Declare the command line options that are hidden from the user
  // (these are used as positional options).
  po::options_description hidden("Hidden options");
  hidden.add_options()
  ("Model",
   po::value(&options.model),
   "one of: hierarchical, s2t, t2s, t2s-scfg, f2s")
  ("TestSetFile",
   po::value(&options.testSetFile),
   "test set file")
  ;

  // Compose the full set of command-line options.
  po::options_description cmdLineOptions;
  cmdLineOptions.add(visible).add(hidden);

  // Register the positional options.
  po::positional_options_description p;
  p.add("Model", 1);
  p.add("TestSetFile", 1);

  // Process the command-line.
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).style(MosesOptionStyle()).
              options(cmdLineOptions).positional(p).run(), vm);
    po::notify(vm);
  } catch (const std::exception &e) {
    std::ostringstream msg;
    msg << e.what() << "\n\n" << visible << usageBottom.str();
    Error(msg.str());
  }

  if (vm.count("help")) {
    std::cout << visible << usageBottom.str() << std::endl;
    std::exit(0);
  }

  // Check all positional options were given.
  if (!vm.count("TestSetFile")) {
    std::ostringstream msg;
    std::cerr << visible << usageBottom.str() << std::endl;
    std::exit(1);
  }
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
