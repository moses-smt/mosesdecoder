// Query binary phrase tables.
// Christian Hardmeier, 16 May 2010

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Util.h"
#include "OnDiskWrapper.h"
#include "SourcePhrase.h"

using namespace std;
using namespace OnDiskPt;

void usage();

typedef unsigned int uint;

void Tokenize(OnDiskPt::Phrase &phrase
              , const std::string &token, bool addSourceNonTerm, bool addTargetNonTerm
              , OnDiskPt::OnDiskWrapper &onDiskWrapper)
{

  bool nonTerm = false;
  size_t tokSize = token.size();
  int comStr =token.compare(0, 1, "[");

  if (comStr == 0) {
    comStr = token.compare(tokSize - 1, 1, "]");
    nonTerm = comStr == 0;
  }

  if (nonTerm) {
    // non-term
    size_t splitPos		= token.find_first_of("[", 2);
    string wordStr	= token.substr(0, splitPos);

    if (splitPos == string::npos) {
      // lhs - only 1 word
      Word *word = new Word();
      word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
      phrase.AddWord(word);
    } else {
      // source & target non-terms
      if (addSourceNonTerm) {
        Word *word = new Word();
        word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
        phrase.AddWord(word);
      }

      wordStr = token.substr(splitPos, tokSize - splitPos);
      if (addTargetNonTerm) {
        Word *word = new Word();
        word->CreateFromString(wordStr, onDiskWrapper.GetVocab());
        phrase.AddWord(word);
      }

    }
  } else {
    // term
    Word *word = new Word();
    word->CreateFromString(token, onDiskWrapper.GetVocab());
    phrase.AddWord(word);
  }
}


int main(int argc, char **argv)
{
  int tableLimit = 20;
  std::string ttable = "";
  bool useAlignments = false;

  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-tlimit")) {
      if(i + 1 == argc)
        usage();
      tableLimit = atoi(argv[++i]);
    } else if(!strcmp(argv[i], "-t")) {
      if(i + 1 == argc)
        usage();
      ttable = argv[++i];
    }
    else
      usage();
  }

  if(ttable == "")
    usage();

	OnDiskWrapper onDiskWrapper;
  bool retDb = onDiskWrapper.BeginLoad(ttable);
	CHECK(retDb);
	
	cerr << "Ready..." << endl;
	
  std::string line;
  while(getline(std::cin, line)) {
    std::vector<std::string> tokens;
    tokens = Moses::Tokenize(line, " ");

		cerr << "line: " << line << endl;
		
		// create source phrase
    SourcePhrase sourcePhrase;

		for (size_t pos = 0; pos < tokens.size(); ++pos)
		{
		  const string &tok = tokens[pos];
		  
		  if (pos == tokens.size() - 1) 
		  { // last position. LHS non-term
			  Tokenize(sourcePhrase, tok, false, true, onDiskWrapper);
			}
			else
			{
			  Tokenize(sourcePhrase, tok, true, true, onDiskWrapper);
			}
		}
		
    const PhraseNode *node = &onDiskWrapper.GetRootSourceNode();
		cerr << "node=" << node << endl;
    assert(node);
    
    for (size_t pos = 0; pos < sourcePhrase.GetSize(); ++pos)
		{
		  const Word &word = sourcePhrase.GetWord(pos);
		  cerr << word << " ";
		  node = node->GetChild(word, onDiskWrapper);
  		cerr << "node=" << node << endl;
		  
		  if (node == NULL)
		  {
		    break;
		  }
		}
    
    if (node)
    { // source phrase points to a bunch of rules
      const TargetPhraseCollection *coll = node->GetTargetPhraseCollection(tableLimit, onDiskWrapper);
      string str = coll->GetDebugStr();
      cout << "Found " << coll->GetSize() << endl;
      
      for (size_t ind = 0; ind < coll->GetSize(); ++ind)
      {
        const TargetPhrase &targetPhrase = coll->GetTargetPhrase(ind);
        cerr << "  ";
        targetPhrase.DebugPrint(cerr, onDiskWrapper.GetVocab());
        cerr << endl;
        

      }
    }
    else
    {
      cout << "Not found" << endl;
    }
    
    std::cout << '\n';
    std::cout.flush();
  }
  
  cerr << "Finished." << endl;
	
}

void usage()
{
  std::cerr << 	"Usage: queryOnDiskPt [-n <nscores>] [-a] -t <ttable>\n"
            "-tlimit <table limit>      max number of rules per source phrase (default: 20)\n"
            "-t <ttable>       phrase table\n";
  exit(1);
}
