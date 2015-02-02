#include <iostream>

#ifdef WITH_THREADS
#include <boost/thread/thread.hpp>
#endif

#include "moses/TypeDef.h"
#include "moses/TranslationModel/CompactPT/PhraseTableCreator.h"

#include "util/file.hh"

using namespace Moses;

void printHelp(char **argv)
{
  std::cerr << "Usage " << argv[0] << ":\n"
            "  options: \n"
            "\t-in  string       -- input table file name\n"
            "\t-out string       -- prefix of binary table file\n"
            "\t-T string         -- path to temporary directory (uses /tmp by default)\n"
            "\t-nscores int      -- number of score components in phrase table\n"
            "\t-no-alignment-info   -- do not include alignment info in the binary phrase table\n"
#ifdef WITH_THREADS
            "\t-threads int|all  -- number of threads used for conversion\n"
#endif
            "\n  advanced:\n"
            "\t-encoding string  -- encoding type: PREnc REnc None (default PREnc)\n"
            "\t-rankscore int    -- score index of P(t|s) (default 2)\n"
            "\t-maxrank int      -- maximum rank for PREnc (default 100)\n"
            "\t-landmark int     -- use landmark phrase every 2^n source phrases (default 10)\n"
            "\t-fingerprint int  -- number of bits used for source phrase fingerprints (default 16)\n"
            "\t-join-scores      -- single set of Huffman codes for score components\n"
            "\t-quantize int     -- maximum number of scores per score component\n"
            "\t-no-warnings      -- suppress warnings about missing alignment data\n"
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


int main(int argc, char **argv)
{

  std::string inFilePath;
  std::string outFilePath("out");
  std::string tempfilePath;
  PhraseTableCreator::Coding coding = PhraseTableCreator::PREnc;

  size_t numScoreComponent = 4;
  size_t orderBits = 10;
  size_t fingerprintBits = 16;
  bool useAlignmentInfo = true;
  bool multipleScoreTrees = true;
  size_t quantize = 0;
  size_t maxRank = 100;
  bool sortScoreIndexSet = false;
  size_t sortScoreIndex = 2;
  bool warnMe = true;
  size_t threads = 1;

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
    } else if("-encoding" == arg && i+1 < argc) {
      ++i;
      std::string val(argv[i]);
      if(val == "None" || val == "none") {
        coding = PhraseTableCreator::None;
      } else if(val == "REnc" || val == "renc") {
        coding = PhraseTableCreator::REnc;
      } else if(val == "PREnc" || val == "prenc") {
        coding = PhraseTableCreator::PREnc;
      }
    } else if("-maxrank" == arg && i+1 < argc) {
      ++i;
      maxRank = atoi(argv[i]);
    } else if("-nscores" == arg && i+1 < argc) {
      ++i;
      numScoreComponent = atoi(argv[i]);
    } else if("-rankscore" == arg && i+1 < argc) {
      ++i;
      sortScoreIndex = atoi(argv[i]);
      sortScoreIndexSet = true;
    } else if("-no-alignment-info" == arg) {
      useAlignmentInfo = false;
    } else if("-landmark" == arg && i+1 < argc) {
      ++i;
      orderBits = atoi(argv[i]);
    } else if("-fingerprint" == arg && i+1 < argc) {
      ++i;
      fingerprintBits = atoi(argv[i]);
    } else if("-join-scores" == arg) {
      multipleScoreTrees = false;
    } else if("-quantize" == arg && i+1 < argc) {
      ++i;
      quantize = atoi(argv[i]);
    } else if("-no-warnings" == arg) {
      warnMe = false;
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
      //something's wrong... print help
      printHelp(argv);
      return 1;
    }
  }

  if(!sortScoreIndexSet && numScoreComponent != 4 && coding == PhraseTableCreator::PREnc) {
    std::cerr << "WARNING: You are using a nonstandard number of scores ("
              << numScoreComponent << ") with PREnc. Set the index of P(t|s) "
              "with  -rankscore int  if it is not "
              << sortScoreIndex << "." << std::endl;
  }

  if(sortScoreIndex >= numScoreComponent) {
    std::cerr << "ERROR: -rankscore " << sortScoreIndex << " is out of range (0 ... "
              << (numScoreComponent-1) << ")" << std::endl;
    abort();
  }

  if(outFilePath.rfind(".minphr") != outFilePath.size() - 7)
    outFilePath += ".minphr";

  PhraseTableCreator(inFilePath, outFilePath, tempfilePath,
                     numScoreComponent, sortScoreIndex,
                     coding, orderBits, fingerprintBits,
                     useAlignmentInfo, multipleScoreTrees,
                     quantize, maxRank, warnMe
#ifdef WITH_THREADS
                     , threads
#endif
                    );
}
