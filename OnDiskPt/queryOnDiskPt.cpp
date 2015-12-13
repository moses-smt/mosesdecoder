// Query binary phrase tables.
// Christian Hardmeier, 16 May 2010

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "moses/Util.h"
#include "OnDiskWrapper.h"
#include "SourcePhrase.h"
#include "OnDiskQuery.h"

using namespace std;
using namespace OnDiskPt;

void usage();

typedef unsigned int uint;

int main(int argc, char **argv)
{
  int tableLimit = 20;
  std::string ttable = "";
  // bool useAlignments = false;

  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-tlimit")) {
      if(i + 1 == argc)
        usage();
      tableLimit = atoi(argv[++i]);
    } else if(!strcmp(argv[i], "-t")) {
      if(i + 1 == argc)
        usage();
      ttable = argv[++i];
    } else
      usage();
  }

  if(ttable == "")
    usage();

  OnDiskWrapper onDiskWrapper;
  onDiskWrapper.BeginLoad(ttable);
  OnDiskQuery onDiskQuery(onDiskWrapper);

  cerr << "Ready..." << endl;

  std::string line;
  while(getline(std::cin, line)) {
    std::vector<std::string> tokens;
    tokens = Moses::Tokenize(line, " ");

    cerr << "line: " << line << endl;
    const PhraseNode* node = onDiskQuery.Query(tokens);

    if (node) {
      // source phrase points to a bunch of rules
      TargetPhraseCollection::shared_ptr coll = node->GetTargetPhraseCollection(tableLimit, onDiskWrapper);
      string str = coll->GetDebugStr();
      cout << "Found " << coll->GetSize() << endl;

      for (size_t ind = 0; ind < coll->GetSize(); ++ind) {
        const TargetPhrase &targetPhrase = coll->GetTargetPhrase(ind);
        cerr << "  ";
        targetPhrase.DebugPrint(cerr, onDiskWrapper.GetVocab());
        cerr << endl;
      }
    } else {
      cout << "Not found" << endl;
    }

    std::cout << '\n';
    std::cout.flush();
  }

  cerr << "Finished." << endl;
}

void usage()
{
  std::cerr << "Usage: queryOnDiskPt [-n <nscores>] [-a] -t <ttable>\n"
            "-tlimit <table limit>      max number of rules per source phrase (default: 20)\n"
            "-t <ttable>       phrase table\n";
  exit(1);
}
