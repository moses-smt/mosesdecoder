#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <cstdlib>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

#include "moses/AlignmentInfoCollection.h"
#include "moses/FF/ESM-Feature/UtilESM.h"
#include "util/file.hh"
#include "util/file_piece.hh"
#include "util/tokenize_piece.hh"
#include "util/string_piece.hh"


int main(int argc, char * argv[]) {

  namespace po = boost::program_options;
  po::options_description options("Edit Sequence Model building options");
  
  std::string sourcePath;
  std::string targetPath;
  std::string alignmentPath;
  
  options.add_options()
    ("help,h", po::bool_switch(), "Show this help message")
    ("source", po::value<std::string>(&sourcePath)
#if BOOST_VERSION >= 104200
       ->required()
#endif
     , "Tokenized source text")
    ("target", po::value<std::string>(&targetPath)
#if BOOST_VERSION >= 104200
       ->required()
#endif
     , "Tokenized target text")
    ("alignment", po::value<std::string>(&alignmentPath)
#if BOOST_VERSION >= 104200
       ->required()
#endif
     , "Word alignment file");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);

  if (argc == 1 || vm["help"].as<bool>()) {
    std::cerr << options << std::endl;
    return 1;
  }

  po::notify(vm);

  // required() appeared in Boost 1.42.0.
#if BOOST_VERSION < 104200
  if (!vm.count("source")) {
    std::cerr << "the option '--source' is required but missing" << std::endl;
    return 1;
  }
  if (!vm.count("target")) {
    std::cerr << "the option '--target' is required but missing" << std::endl;
    return 1;
  }
  if (!vm.count("alignment")) {
    std::cerr << "the option '--alignment' is required but missing" << std::endl;
    return 1;
  }
#endif

  util::FilePiece sourceFP(util::OpenReadOrThrow(sourcePath.c_str()), "parallel corpus", &std::cerr);
  util::FilePiece targetFP(util::OpenReadOrThrow(targetPath.c_str()));
  util::FilePiece alignmentFP(util::OpenReadOrThrow(alignmentPath.c_str()));
  
  bool delimiters[256];
  util::BoolCharacter::Build("\0\t\n\r ", delimiters);
  
  bool aDelimiters[256];
  util::BoolCharacter::Build("\0\t\n\r- ", aDelimiters);
  
  try {
    while(true) {
      StringPiece sourceStr(sourceFP.ReadLine());
      StringPiece targetStr(targetFP.ReadLine());
      StringPiece alignmentStr(alignmentFP.ReadLine());
            
      std::vector<std::string> source;
      for(util::TokenIter<util::BoolCharacter, true> w(sourceStr, delimiters); w; ++w)
        source.push_back(w->as_string());
      
      std::vector<std::string> target;
      for(util::TokenIter<util::BoolCharacter, true> w(targetStr, delimiters); w; ++w)
        target.push_back(w->as_string());

      std::set<std::pair<size_t, size_t> > container;
      for(util::TokenIter<util::BoolCharacter, true> w(alignmentStr, aDelimiters); w; ++w) {
        size_t a = boost::lexical_cast<size_t>((w++)->as_string());
        size_t b = boost::lexical_cast<size_t>(w->as_string());
        container.insert(std::make_pair(a, b));
      }
      const Moses::AlignmentInfo* alignmentPtr
        = Moses::AlignmentInfoCollection::Instance().Add(container);
      
      std::vector<std::string> edits;
      Moses::calculateEdits(edits, source, target, *alignmentPtr);
  
      BOOST_FOREACH(std::string edit, edits)
          std::cout << edit << " ";
      std::cout << std::endl;
    }
  } catch (const util::EndOfFileException &e) {}

  
  return 0;
}