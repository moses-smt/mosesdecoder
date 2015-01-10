#include "FilterRuleTable.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

#include <boost/program_options.hpp>

#include "util/string_piece.hh"
#include "util/string_piece_hash.hh"
#include "util/tokenize_piece.hh"

#include "syntax-common/exception.h"
#include "syntax-common/xml_tree_parser.h"

#include "InputFileStream.h"

#include "Options.h"
#include "StringBasedFilter.h"
#include "TreeBasedFilter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

int FilterRuleTable::Main(int argc, char *argv[])
{
  // Process command-line options.
  Options options;
  ProcessOptions(argc, argv, options);

  // Open input file.
  Moses::InputFileStream testStream(options.testSetFile);

  // Read the first test sentence and determine if it is a parse tree or a
  // string.
  std::string line;
  if (!std::getline(testStream, line)) {
    // TODO Error?
    return 0;
  }
  if (line.find_first_of('<') == std::string::npos) {
    // Test sentences are strings.
    std::vector<std::vector<std::string> > sentences;
    do {
      sentences.resize(sentences.size()+1);
      ReadTokens(line, sentences.back());
    } while (std::getline(testStream, line));
    StringBasedFilter filter(sentences);
    filter.Filter(std::cin, std::cout);
  } else {
    // Test sentences are XML parse trees.
    XmlTreeParser parser;
    std::vector<boost::shared_ptr<StringTree> > sentences;
    int lineNum = 1;
    do {
      if (line.size() == 0) {
        std::cerr << "skipping blank test sentence at line " << lineNum
                  << std::endl;
        continue;
      }
      sentences.push_back(boost::shared_ptr<StringTree>(parser.Parse(line)));
      ++lineNum;
    } while (std::getline(testStream, line));
    TreeBasedFilter filter(sentences);
    filter.Filter(std::cin, std::cout);
  }

  return 0;
}

void FilterRuleTable::ReadTokens(const std::string &s,
                                 std::vector<std::string> &tokens)
{
  tokens.clear();
// TODO
}

void FilterRuleTable::ProcessOptions(int argc, char *argv[],
                                     Options &options) const
{
  namespace po = boost::program_options;
  namespace cls = boost::program_options::command_line_style;

  // Construct the 'top' of the usage message: the bit that comes before the
  // options list.
  std::ostringstream usageTop;
  usageTop << "Usage: " << GetName()
           << " [OPTION]... TEST\n\n"
           << "Given a SCFG/STSG rule table (on standard input) and a set of test sentences,\nfilter out the rules that cannot be applied to any of the test sentences and\nwrite the filtered table to standard output.\n\n"
           << "Options";

  // Construct the 'bottom' of the usage message.
  std::ostringstream usageBottom;
  usageBottom << "TODO";

  // Declare the command line options that are visible to the user.
  po::options_description visible(usageTop.str());

  // Declare the command line options that are hidden from the user
  // (these are used as positional options).
  po::options_description hidden("Hidden options");
  hidden.add_options()
  ("TestSetFile",
   po::value(&options.testSetFile),
   "test set file")
  ;

  // Compose the full set of command-line options.
  po::options_description cmdLineOptions;
  cmdLineOptions.add(visible).add(hidden);

  // Register the positional options.
  po::positional_options_description p;
  p.add("TestSetFile", 1);

  // Process the command-line.
  po::variables_map vm;
  const int optionStyle = cls::allow_long
                          | cls::long_allow_adjacent
                          | cls::long_allow_next;
  try {
    po::store(po::command_line_parser(argc, argv).style(optionStyle).
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

void FilterRuleTable::Error(const std::string &msg) const
{
  std::cerr << GetName() << ": " << msg << std::endl;
  std::exit(1);
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
