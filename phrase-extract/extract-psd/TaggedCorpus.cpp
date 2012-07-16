#include <string>
#include <vector>
#include "TaggedCorpus.h"
#include "Util.h"

using namespace std;
using namespace Moses;

vector< vector<string> > parseTaggedString(const string &input, const string &delimiter){
  vector< vector<string> > taggedWords;
  vector<string> words = Tokenize(input," ");
  for(int i =0; i < words.size(); i++)  {
    string& word = words[i];
    vector<string> tags = Tokenize(word, delimiter);
    taggedWords.push_back(tags);
  }
  return taggedWords;
}
