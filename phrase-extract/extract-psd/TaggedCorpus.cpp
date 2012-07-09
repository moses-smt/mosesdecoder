#include <string>
#include <vector>
#include "TaggedCorpus.h"
#include "StringUtils.h"

using namespace std;

vector< vector<string> > parseTaggedString(const string &input, const string &delimiter){
  vector< vector<string> > taggedWords;
  vector<string> words = tokenizeString(input," ");
  for(int i =0; i < words.size(); i++)  {
    string& word = words[i];
    vector<string> tags = tokenizeString(word,delimiter);
    // this was for MADA output only
    /*    if (tags.size() < 6){
      for(int i = tags.size(); i < 6; i++){
	tags.push_back(tags[0]);
      }
    }
    */
    taggedWords.push_back(tags);
  }
  return taggedWords;
}
