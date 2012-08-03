#include <iostream>
#include "TypeDef.h"
#include "CompactPT/PhraseTableCreator.h"
#include "CompactPT/CanonicalHuffman.h"

using namespace Moses;

void printHelp(char **argv) {
  std::cerr << "Usage " << argv[0] << ":\n"
            "options: \n"
            "\t-in  string       -- input table file name\n"
            "\t-out string       -- prefix of binary table file\n"
            "\t-encoding string  -- Encoding type (PREnc REnc None)\n"
            "\t-maxrank int      -- Maximum rank for PREnc\n"
            "\t-nscores int      -- number of score components in phrase table\n"
            "\t-alignment-info   -- include alignment info in the binary phrase table\n"
            "\nadvanced:\n"
            "\t-landmark int     -- use landmark phrase every 2^n source phrases\n"
            "\t-fingerprint int  -- number of bits used for source phrase fingerprints\n"
            "\t-join-scores      -- single set of Huffman codes for score components\n"
            "\t-quantize int     -- maximum number of scores per score component\n"

#ifdef WITH_THREADS
            "\t-threads int      -- number of threads used for conversion\n"
#endif 
            "\n\n"
            
            "  For more information see: http://www.statmt.org/moses/...\n"
            "  and\n\n"
            "  @article { junczys_mtm_2012,\n"
            "      author = { Marcin Junczys-Dowmunt },\n"
            "      title = { Phrasal Rank-Encoding: Exploiting Phrase Redundancy and\n"
            "                Translational Relations for Phrase Table Compression },\n"
            "      journal = { The Prague Bulletin of Mathematical Linguistics },\n"
            "      volume = { 98 },\n"
            "      year = { 2012 },\n"
            "      note = { Proceedings of the MT Marathon 2012, Edinburgh },\n"
            "  }\n\n";
}


int main(int argc, char **argv) {
    
  std::string inFilePath;
  std::string outFilePath("out");
  PhraseTableCreator::Coding coding = PhraseTableCreator::PREnc;
  
  size_t numScoreComponent = 5;  
  size_t orderBits = 10;
  size_t fingerprintBits = 16;
  bool useAlignmentInfo = false;
  bool multipleScoreTrees = true;
  size_t quantize = 0;
  size_t maxRank = 100;
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
    }
    else if("-out" == arg && i+1 < argc) {
      ++i;
      outFilePath = argv[i];
    }
    else if("-encoding" == arg && i+1 < argc) {
      ++i;
      std::string val(argv[i]);
      if(val == "None" || val == "none") {
        coding = PhraseTableCreator::None;
      }
      else if(val == "REnc" || val == "renc") {
        coding = PhraseTableCreator::REnc;
      }
      else if(val == "PREnc" || val == "prenc") {
        coding = PhraseTableCreator::PREnc;
      }
    }
    else if("-maxrank" == arg && i+1 < argc) {
      ++i;
      maxRank = atoi(argv[i]);
    }
    else if("-nscores" == arg && i+1 < argc) {
      ++i;
      numScoreComponent = atoi(argv[i]);
    }
    else if("-alignment-info" == arg) {
      useAlignmentInfo = true;
    }
    else if("-landmark" == arg && i+1 < argc) {
      ++i;
      orderBits = atoi(argv[i]);
    }
    else if("-fingerprint" == arg && i+1 < argc) {
      ++i;
      fingerprintBits = atoi(argv[i]);
    }
    else if("-join-scores" == arg) {
      multipleScoreTrees = false;
    }
    else if("-quantize" == arg && i+1 < argc) {
      ++i;
      quantize = atoi(argv[i]);
    }
    else if("-threads" == arg && i+1 < argc) {
#ifdef WITH_THREADS
      ++i;
      threads = atoi(argv[i]);
#else
      std::cerr << "Thread support not compiled in" << std::endl;
      exit(1);
#endif
    }
    else {
      //somethings wrong... print help
      printHelp(argv);
      return 1;
    }
  }
  
  if(outFilePath.rfind(".minphr") != outFilePath.size() - 7)
    outFilePath += ".minphr";
  
  PhraseTableCreator(inFilePath, outFilePath, numScoreComponent,
                     coding, orderBits, fingerprintBits,
                     useAlignmentInfo, multipleScoreTrees,
                     quantize, maxRank
#ifdef WITH_THREADS
                     , threads
#endif                     
                     );
}
