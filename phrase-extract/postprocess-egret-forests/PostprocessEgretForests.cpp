#include "PostprocessEgretForests.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "syntax-common/exception.h"

#include "Forest.h"
#include "ForestParser.h"
#include "ForestWriter.h"
#include "Options.h"
#include "SplitPoint.h"
#include "SplitPointFileParser.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

int PostprocessEgretForests::Main(int argc, char *argv[])
{
  try {
    // Process command-line options.
    Options options;
    ProcessOptions(argc, argv, options);

    // Open input files.
    boost::scoped_ptr<SplitPointFileParser> splitPointParser;
    std::ifstream splitPointFileStream;
    if (!options.splitPointsFile.empty()) {
      OpenInputFileOrDie(options.splitPointsFile, splitPointFileStream);
      splitPointParser.reset(new SplitPointFileParser(splitPointFileStream));
    }

    ProcessForest(std::cin, std::cout, splitPointParser.get(), options);
  } catch (const MosesTraining::Syntax::Exception &e) {
    Error(e.msg());
  }
  return 0;
}

void PostprocessEgretForests::ProcessForest(
  std::istream &in, std::ostream &out, SplitPointFileParser *splitPointParser,
  const Options &options)
{
  std::size_t sentNum = 0;
  ForestWriter writer(options, out);
  ForestParser end;
  for (ForestParser p(in); p != end; ++p) {
    ++sentNum;
    if (splitPointParser) {
      if (*splitPointParser == SplitPointFileParser()) {
        throw Exception("prematurely reached end of split point file");
      }
      if (!p->forest.vertices.empty()) {
        try {
          MarkSplitPoints((*splitPointParser)->splitPoints, p->forest);
          MarkSplitPoints((*splitPointParser)->splitPoints, p->sentence);
        } catch (const Exception &e) {
          std::ostringstream msg;
          msg << "failed to mark split point for sentence " << sentNum << ": "
              << e.msg();
          throw Exception(msg.str());
        }
      }
      ++(*splitPointParser);
    }
    writer.Write(p->sentence, p->forest, p->sentNum);
  }
}

void PostprocessEgretForests::ProcessOptions(int argc, char *argv[],
    Options &options) const
{
  namespace po = boost::program_options;
  namespace cls = boost::program_options::command_line_style;

  // Construct the 'top' of the usage message: the bit that comes before the
  // options list.
  std::ostringstream usageTop;
  usageTop << "Usage: " << name()
           << " [OPTION]...\n\n"
           << "TODO\n\n"
           << "Options";

  // Construct the 'bottom' of the usage message.
  std::ostringstream usageBottom;
  usageBottom << "TODO";

  // Declare the command line options that are visible to the user.
  po::options_description visible(usageTop.str());
  visible.add_options()
  ("Escape",
   "escape Moses special characters")
  ("MarkSplitPoints",
   po::value(&options.splitPointsFile),
   "read split points from named file and mark (using @) in output")
  ;

  // Declare the command line options that are hidden from the user
  // (these are used as positional options).
  po::options_description hidden("Hidden options");
  hidden.add_options()
  // None
  ;

  // Compose the full set of command-line options.
  po::options_description cmdLineOptions;
  cmdLineOptions.add(visible).add(hidden);

  // Register the positional options.
  po::positional_options_description p;
  // Currently none

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

  // Process Boolean options.
  if (vm.count("Escape")) {
    options.escape = true;
  }
}

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
