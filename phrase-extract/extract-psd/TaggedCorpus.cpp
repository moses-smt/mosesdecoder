#include <string>
#include <vector>
#include "TaggedCorpus.h"
#include "Util.h"

using namespace std;
using namespace Moses;

vector< vector<string> > parseTaggedString(const string &input, const string &delimiter, size_t factorCount){
  vector< vector<string> > taggedWords;
  vector<string> words = Tokenize(input," ");
  for(int i =0; i < words.size(); i++)  {
    string& word = words[i];
    vector<string> tags = Tokenize(word, delimiter);
    if (tags.size() != factorCount) {
      cerr << "error: unexpected number of factors (" << tags.size() << ") in token: " << word << endl;
      exit(1);
    }
    taggedWords.push_back(tags);
  }
  return taggedWords;
}
