#include <iostream>
#include <string>

#include "moses/Timer.h"
#include "moses/InputFileStream.h"
#include "moses/FF/LexicalReordering/LexicalReorderingTable.h"

using namespace Moses;

Timer timer;

void printHelp()
{
  std::cerr << "Usage:\n"
            "options: \n"
            "\t-in  string -- input table file name\n"
            "\t-out string -- prefix of binary table files\n"
            "If -in is not specified reads from stdin\n"
            "\n";
}

int main(int argc, char** argv)
{
  std::cerr << "processLexicalTable v0.1 by Konrad Rawlik\n";
  std::string inFilePath;
  std::string outFilePath("out");
  if(1 >= argc) {
    printHelp();
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
    } else {
      //somethings wrong... print help
      printHelp();
      return 1;
    }
  }

  bool success = false;

  if(inFilePath.empty()) {
    std::cerr << "processing stdin to " << outFilePath << ".*\n";
    success = LexicalReorderingTableTree::Create(std::cin, outFilePath);
  } else {
    std::cerr << "processing " << inFilePath<< " to " << outFilePath << ".*\n";
    InputFileStream file(inFilePath);
    success = LexicalReorderingTableTree::Create(file, outFilePath);
  }

  return (success ? 0 : 1);
}
