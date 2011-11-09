#include "Compactify.h"

#include "NumberedSet.h"
#include "Options.h"
#include "RuleTableParser.h"

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/unordered_map.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

namespace moses {

int Compactify::main(int argc, char *argv[]) {
  // Process the command-line arguments.
  Options options;
  processOptions(argc, argv, options);

  // Open the input stream.
  std::istream *inputPtr;
  std::ifstream inputFileStream;
  if (options.inputFile.empty() || options.inputFile == "-") {
    inputPtr = &(std::cin);
  } else {
    inputFileStream.open(options.inputFile.c_str());
    if (!inputFileStream) {
      std::ostringstream msg;
      msg << "failed to open input file: " << options.inputFile;
      error(msg.str());
    }
    inputPtr = &inputFileStream;
  }
  std::istream &input = *inputPtr;

  // Open the output stream.
  std::ostream *outputPtr;
  std::ofstream outputFileStream;
  if (options.outputFile.empty()) {
    outputPtr = &(std::cout);
  } else {
    outputFileStream.open(options.outputFile.c_str());
    if (!outputFileStream) {
      std::ostringstream msg;
      msg << "failed to open output file: " << options.outputFile;
      error(msg.str());
    }
    outputPtr = &outputFileStream;
  }
  std::ostream &output = *outputPtr;

  // Open a temporary file: the rule section must appear last in the output
  // file, but we don't want to store the full set of rules in memory during
  // processing, so instead they're written to a temporary file then copied to
  // the output file as a final step.
  std::fstream tempFileStream;
  {
    char fileNameTemplate[] = "/tmp/compact_XXXXXX";
    int fd = mkstemp(fileNameTemplate);
    if (fd == -1) {
      std::ostringstream msg;
      msg << "failed to open temporary file with pattern " << fileNameTemplate;
      error(msg.str());
    }
    tempFileStream.open(fileNameTemplate);
    if (!tempFileStream) {
      std::ostringstream msg;
      msg << "failed to open existing temporary file: " << fileNameTemplate;
      error(msg.str());
    }
    // Close the original file descriptor.
    close(fd);
    // Unlink the file.  Its contents will be safe until tempFileStream is
    // closed.
    unlink(fileNameTemplate);
  }

  // Write the version number
  output << "1" << '\n';

  SymbolSet symbolSet;
  PhraseSet sourcePhraseSet;
  PhraseSet targetPhraseSet;
  AlignmentSetSet alignmentSetSet;

  SymbolPhrase symbolPhrase;

  size_t ruleCount = 0;
  RuleTableParser end;
  try {
    for (RuleTableParser parser(input); parser != end; ++parser) {
      const RuleTableParser::Entry &entry = *parser;
      ++ruleCount;

      // Report progress in the same format as extract-rules.
      if (ruleCount % 100000 == 0) {
        std::cerr << "." << std::flush;
      }
      if (ruleCount % 1000000 == 0) {
        std::cerr << " " << ruleCount << std::endl;
      }

      // Encode the source LHS + RHS as a vector of symbol IDs and insert into
      // sourcePhraseSet.
      encodePhrase(entry.sourceLhs, entry.sourceRhs, symbolSet, symbolPhrase);
      SymbolIDType sourceId = sourcePhraseSet.insert(symbolPhrase);

      // Encode the target LHS + RHS as a vector of symbol IDs and insert into
      // targetPhraseSet.
      encodePhrase(entry.targetLhs, entry.targetRhs, symbolSet, symbolPhrase);
      SymbolIDType targetId = targetPhraseSet.insert(symbolPhrase);

      // Insert the alignments into alignmentSetSet.
      AlignmentSetIDType alignmentSetId = alignmentSetSet.insert(
          entry.alignments);

      // Write this rule to the temporary file.
      tempFileStream << sourceId << " " << targetId << " " << alignmentSetId;
      for (std::vector<std::string>::const_iterator p = entry.scores.begin();
           p != entry.scores.end(); ++p) {
        tempFileStream << " " << *p;
      }
      tempFileStream << " :";
      for (std::vector<std::string>::const_iterator p = entry.counts.begin();
           p != entry.counts.end(); ++p) {
        tempFileStream << " " << *p;
      }
      tempFileStream << '\n';
    }
  } catch (Exception &e) {
    std::ostringstream msg;
    msg << "error processing line " << ruleCount+1 << ": " << e.getMsg();
    error(msg.str());
  }

  // Report the counts.

  if (ruleCount % 1000000 != 0) {
    std::cerr << std::endl;
  }
  std::cerr << "Rule count:          " << ruleCount << std::endl;
  std::cerr << "Symbol count:        " << symbolSet.size() << std::endl;
  std::cerr << "Source phrase count: " << sourcePhraseSet.size() << std::endl;
  std::cerr << "Target phrase count: " << targetPhraseSet.size() << std::endl;
  std::cerr << "Alignment set count: " << alignmentSetSet.size() << std::endl;

  // Write the symbol vocabulary.

  output << symbolSet.size() << '\n';
  for (SymbolSet::const_iterator p = symbolSet.begin();
       p != symbolSet.end(); ++p) {
    const std::string &str = **p;
    output << str << '\n';
  }

  // Write the source phrases.

  output << sourcePhraseSet.size() << '\n';
  for (PhraseSet::const_iterator p = sourcePhraseSet.begin();
       p != sourcePhraseSet.end(); ++p) {
    const SymbolPhrase &sourcePhrase = **p;
    for (SymbolPhrase::const_iterator q = sourcePhrase.begin();
         q != sourcePhrase.end(); ++q) {
      if (q != sourcePhrase.begin()) {
        output << " ";
      }
      output << *q;
    }
    output << '\n';
  }

  // Write the target phrases.

  output << targetPhraseSet.size() << '\n';
  for (PhraseSet::const_iterator p = targetPhraseSet.begin();
       p != targetPhraseSet.end(); ++p) {
    const SymbolPhrase &targetPhrase = **p;
    for (SymbolPhrase::const_iterator q = targetPhrase.begin();
         q != targetPhrase.end(); ++q) {
      if (q != targetPhrase.begin()) {
        output << " ";
      }
      output << *q;
    }
    output << '\n';
  }

  // Write the alignment sets.

  output << alignmentSetSet.size() << '\n';
  for (AlignmentSetSet::const_iterator p = alignmentSetSet.begin();
       p != alignmentSetSet.end(); ++p) {
    const AlignmentSet &alignmentSet = **p;
    for (AlignmentSet::const_iterator q = alignmentSet.begin();
          q != alignmentSet.end(); ++q) {
      if (q != alignmentSet.begin()) {
        output << " ";
      }
      output << q->first << "-" << q->second;
    }
    output << '\n';
  }

  // Write the rule count.
  output << ruleCount << '\n';

  // Copy the rules from the temporary file.
  tempFileStream.seekg(0);
  std::string line;
  while (std::getline(tempFileStream, line)) {
    output << line << '\n';
  }

  return 0;
}

void Compactify::processOptions(int argc, char *argv[],
                                Options &options) const {
  namespace po = boost::program_options;

  std::ostringstream usageMsg;
  usageMsg << "usage: " << getName() << " [OPTION]... [FILE]";

  // Declare the command line options that are visible to the user.
  std::string caption = usageMsg.str() + std::string("\n\nAllowed options");
  po::options_description visible(caption);
  visible.add_options()
    ("help", "print help message and exit")
    ("output,o", po::value<std::string>(),
                 "write rule table to arg instead of standard output")
  ;

  // Declare the command line options that are hidden from the user
  // (these are used as positional options).
  po::options_description hidden("Hidden options");
  hidden.add_options()
    ("input", po::value<std::string>(), "input file")
  ;

  // Compose the full set of command-line options.
  po::options_description cmdLineOptions;
  cmdLineOptions.add(visible).add(hidden);

  // Register the positional options.
  po::positional_options_description p;
  p.add("input", 1);

  // Process the command-line.
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).
              options(cmdLineOptions).positional(p).run(), vm);
    po::notify(vm);
  } catch (const std::exception &e) {
    std::ostringstream msg;
    msg << e.what() << "\n\n" << visible;
    error(msg.str());
    std::exit(1);
  }

  if (vm.count("help")) {
    std::cout << visible << std::endl;
    std::exit(0);
  }

  // Process positional options.

  if (vm.count("input")) {
    options.inputFile = vm["input"].as<std::string>();
  }

  // Process remaining options.

  if (vm.count("output")) {
    options.outputFile = vm["output"].as<std::string>();
  }
}

void Compactify::encodePhrase(const std::string &lhs, const StringPhrase &rhs,
                              SymbolSet &symbolSet, SymbolPhrase &vec) const {
  vec.clear();
  vec.reserve(rhs.size()+1);
  SymbolIDType id = symbolSet.insert(lhs);
  vec.push_back(id);
  for (std::vector<std::string>::const_iterator p = rhs.begin();
       p != rhs.end(); ++p) {
    SymbolIDType id = symbolSet.insert(*p);
    vec.push_back(id);
  }
}

}  // namespace moses
