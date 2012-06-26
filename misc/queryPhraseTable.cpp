// Query binary phrase tables.
// Christian Hardmeier, 16 May 2010

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "PhraseDictionaryTree.h"
#include "Util.h"

void usage();

typedef unsigned int uint;

int main(int argc, char **argv)
{
  int nscores = 5;
  std::string ttable = "";
  bool useAlignments = false;
  bool reportCounts = false;

  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-n")) {
      if(i + 1 == argc)
        usage();
      nscores = atoi(argv[++i]);
    } else if(!strcmp(argv[i], "-t")) {
      if(i + 1 == argc)
        usage();
      ttable = argv[++i];
    } else if(!strcmp(argv[i], "-a")) {
      useAlignments = true;
    } else if (!strcmp(argv[i], "-c")) {
      reportCounts = true;
    }
    else
      usage();
  }

  if(ttable == "")
    usage();

  Moses::PhraseDictionaryTree ptree(nscores);
  ptree.UseWordAlignment(useAlignments);
  ptree.Read(ttable);

  std::string line;
  while(getline(std::cin, line)) {
    std::vector<std::string> srcphrase;
    srcphrase = Moses::Tokenize<std::string>(line);

    std::vector<Moses::StringTgtCand> tgtcands;
    std::vector<std::string> wordAlignment;

    if(useAlignments)
      ptree.GetTargetCandidates(srcphrase, tgtcands, wordAlignment);
    else
      ptree.GetTargetCandidates(srcphrase, tgtcands);

    if (reportCounts) {
      std::cout << line << " " << tgtcands.size() << "\n";
    } else {
      for(uint i = 0; i < tgtcands.size(); i++) {
        std::cout << line << " |||";
        for(uint j = 0; j < tgtcands[i].first.size(); j++)
          std::cout << ' ' << *tgtcands[i].first[j];
        std::cout << " |||";

        if(useAlignments) {
          std::cout << " " << wordAlignment[i] << " |||";
        }

        for(uint j = 0; j < tgtcands[i].second.size(); j++)
          std::cout << ' ' << tgtcands[i].second[j];
        std::cout << '\n';
      }
      std::cout << '\n';
    }

    std::cout.flush();
  }
}

void usage()
{
  std::cerr << 	"Usage: queryPhraseTable [-n <nscores>] [-a] -t <ttable>\n"
            "-n <nscores>      number of scores in phrase table (default: 5)\n"
            "-c                only report counts of entries\n"
            "-a                binary phrase table contains alignments\n"
            "-t <ttable>       phrase table\n";
  exit(1);
}
