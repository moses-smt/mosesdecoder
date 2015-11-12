#include <iostream>
#include <string>

#include "moses/Phrase.h"
#include "moses/FactorCollection.h"
#include "moses/Timer.h"
#include "moses/InputFileStream.h"
#include "moses/FF/LexicalReordering/LexicalReorderingTable.h"
#include "moses/parameters/OOVHandlingOptions.h"

using namespace Moses;

Timer timer;

void printHelp()
{
  std::cerr << "Usage:\n"
            "options: \n"
            "\t-table file   -- input table file name\n"
            "\t-f     string -- f query phrase\n"
            "\t-e     string -- e query phrase\n"
            "\t-c     string -- context query phrase\n"
            "\n";
}

std::ostream& operator<<(std::ostream& o, Scores s)
{
  for(size_t i = 0; i < s.size(); ++i) {
    o << s[i] << "  ";
  }
  //o << std::endln;
  return o;
};

int main(int argc, char** argv)
{
  std::cerr << "queryLexicalTable v0.2 by Konrad Rawlik\n";
  std::string inFilePath;
  std::string outFilePath("out");
  bool cache = false;
  std::string query_e, query_f, query_c;
  bool use_context = false;
  bool use_e       = false;
  if(1 >= argc) {
    printHelp();
    return 1;
  }
  for(int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if("-table" == arg && i+1 < argc) {
      //std::cerr << "Table is " << argv[i];
      ++i;
      inFilePath = argv[i];
    } else if("-f" == arg && i+1 < argc) {
      ++i;
      //std::cerr << "F is " << argv[i];
      query_f = argv[i];
    } else if("-e" == arg && i+1 < argc) {
      ++i;
      query_e = argv[i];
      use_e = true;
    } else if("-c" == arg) {
      if(i+1 < argc && '-' != argv[i+1][0]) {
        ++i;
        query_c = argv[i];
        use_context = true;
      } else {
        use_context = false;
      }
    } else if("-cache" == arg) {
      ++i;
      cache = true;
    } else {
      //somethings wrong... print help
      printHelp();
      return 1;
    }
  }

  FactorList f_mask;
  FactorList e_mask;
  FactorList c_mask;
  f_mask.push_back(0);
  if(use_e) {
    e_mask.push_back(0);
  }
  if(use_context) {
    c_mask.push_back(0);
  }
  Phrase e( 0),f(0),c(0);
  // e.CreateFromString(Output, e_mask, query_e, "|", NULL);
  // f.CreateFromString(Input, f_mask, query_f, "|", NULL);
  // c.CreateFromString(Input, c_mask,  query_c,"|", NULL);
  // Phrase.CreateFromString() calls Word.CreateFromSting(), which gets
  // the factor delimiter from StaticData, so it should not be hardcoded
  // here. [UG], thus:
  e.CreateFromString(Output, e_mask, query_e, NULL);
  f.CreateFromString(Input, f_mask, query_f, NULL);
  c.CreateFromString(Input, c_mask,  query_c, NULL);
  LexicalReorderingTable* table;
  if(FileExists(inFilePath+".binlexr.idx")) {
    std::cerr << "Loading binary table...\n";
    table = new LexicalReorderingTableTree(inFilePath, f_mask, e_mask, c_mask);
  } else {
    std::cerr << "Loading ordinary table...\n";
    table = new LexicalReorderingTableMemory(inFilePath, f_mask, e_mask, c_mask);
  }
  //table->DbgDump(&std::cerr);
  if(cache) {
    std::cerr << "Caching for f\n";
    table->InitializeForInputPhrase(f);
  }

  std::cerr << "Querying: "
            << "f='" << f.GetStringRep(f_mask) <<"' "
            << "e='" << e.GetStringRep(e_mask) << "' "
            << "c='" << c.GetStringRep(c_mask) << "'\n";
  std::cerr << table->GetScore(f,e,c) << "\n";
  //table->DbgDump(&std::cerr);
  delete table;
  return 0;
}


