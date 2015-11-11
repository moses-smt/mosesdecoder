#include <iostream>
#include <string>

#ifdef WITH_THREADS
#include <boost/thread/thread.hpp>
#endif

#include "moses/TranslationModel/CompactPT/LexicalReorderingTableCreator.h"

#include "util/file.hh"

using namespace Moses;

void printHelp(char **argv)
{
  std::cerr << "Usage " << argv[0] << ":\n"
            "  options: \n"
            "\t-in  string       -- input table file name\n"
            "\t-out string       -- prefix of binary table file\n"
            "\t-T string         -- path to temporary directory (uses /tmp by default)\n"
#ifdef WITH_THREADS
            "\t-threads int|all  -- number of threads used for conversion\n"
#endif
            "\n  advanced:\n"
            "\t-landmark int     -- use landmark phrase every 2^n phrases\n"
            "\t-fingerprint int  -- number of bits used for phrase fingerprints\n"
            "\t-join-scores      -- single set of Huffman codes for score components\n"
            "\t-quantize int     -- maximum number of scores per score component\n"
            "\n"
            "  For more information see: http://www.statmt.org/moses/?n=Moses.AdvancedFeatures#ntoc6\n\n"
            "  If you use this please cite:\n\n"
            "  @article { junczys_pbml98_2012,\n"
            "      author = { Marcin Junczys-Dowmunt },\n"
            "      title = { Phrasal Rank-Encoding: Exploiting Phrase Redundancy and\n"
            "                Translational Relations for Phrase Table Compression },\n"
            "      journal = { The Prague Bulletin of Mathematical Linguistics },\n"
            "      volume = { 98 },\n"
            "      year = { 2012 },\n"
            "      note = { Proceedings of the MT Marathon 2012, Edinburgh },\n"
            "  }\n\n"
            "  Acknowledgments: Part of this research was carried out at and funded by\n"
            "  the World Intellectual Property Organization (WIPO) in Geneva.\n\n";
}

int main(int argc, char** argv)
{

  std::string inFilePath;
  std::string outFilePath("out");
  std::string tempfilePath;

  size_t orderBits = 10;
  size_t fingerPrintBits = 16;
  bool multipleScoreTrees = true;
  size_t quantize = 0;

  size_t threads =
#ifdef WITH_THREADS
    boost::thread::hardware_concurrency() ? boost::thread::hardware_concurrency() :
#endif
    1;

  if(1 >= argc) {
    printHelp(argv);
    return 1;
  }
  for(int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if("-in" == arg && i+1 < argc) {
      ++i;
      inFilePath = argv[i];
    } else if("-out" == arg && i+1 < argc) {
      ++i;
      outFilePath = argv[i];
    } else if("-T" == arg && i+1 < argc) {
      ++i;
      tempfilePath = argv[i];
      util::NormalizeTempPrefix(tempfilePath);
    } else if("-landmark" == arg && i+1 < argc) {
      ++i;
      orderBits = atoi(argv[i]);
    } else if("-fingerprint" == arg && i+1 < argc) {
      ++i;
      fingerPrintBits = atoi(argv[i]);
    } else if("-join-scores" == arg) {
      multipleScoreTrees = false;
    } else if("-quantize" == arg && i+1 < argc) {
      ++i;
      quantize = atoi(argv[i]);
    } else if("-threads" == arg && i+1 < argc) {
#ifdef WITH_THREADS
      ++i;
      if(std::string(argv[i]) == "all") {
        threads = boost::thread::hardware_concurrency();
        if(!threads) {
          std::cerr << "Could not determine number of hardware threads, setting to 1" << std::endl;
          threads = 1;
        }
      } else
        threads = atoi(argv[i]);
#else
      std::cerr << "Thread support not compiled in" << std::endl;
      exit(1);
#endif
    } else {
      //somethings wrong... print help
      printHelp(argv);
      return 1;
    }
  }

  if(outFilePath.rfind(".minlexr") != outFilePath.size() - 8)
    outFilePath += ".minlexr";

  LexicalReorderingTableCreator(
    inFilePath, outFilePath, tempfilePath,
    orderBits, fingerPrintBits,
    multipleScoreTrees, quantize
#ifdef WITH_THREADS
    , threads
#endif
  );
}
