#include <fstream>

#include "moses/Util.h"
#include "util/exception.hh"

#include "SparseReordering.h"


using namespace std;

namespace Moses 
{

SparseReordering::SparseReordering(const map<string,string>& config) 
{
  static const string kSource= "source";
  static const string kTarget = "target";
  for (map<string,string>::const_iterator i = config.begin(); i != config.end(); ++i) {
    vector<string> fields = Tokenize(i->first, "-");
    if (fields[0] == "words") {
      UTIL_THROW_IF(!(fields.size() == 3), util::Exception, "Sparse reordering word list name should be sparse-words-(source|target)-<id>");
      if (fields[1] == kSource) {
        ReadWordList(i->second,fields[2],&m_sourceWordLists);
      } else if (fields[1] == kTarget) {
        ReadWordList(i->second,fields[2],&m_targetWordLists);
      } else {
        UTIL_THROW(util::Exception, "Sparse reordering requires source or target, not " << fields[1]);
      }
    } else if (fields[0] == "clusters") {
      UTIL_THROW(util::Exception, "Sparse reordering does not yet support clusters" << i->first);
    } else if (fields[0] == "phrase") {
      m_usePhrase = true;
    } else if (fields[0] == "stack") {
      m_useStack = true;
    } else if (fields[0] == "between") {
      m_useBetween = true;
    } else {
      UTIL_THROW(util::Exception, "Unable to parse sparse reordering option: " << i->first);
    }
  }
}

void SparseReordering::ReadWordList(const string& filename, const string& id, vector<WordList>* pWordLists) {
  ifstream fh(filename.c_str());
  string line;
  pWordLists->push_back(WordList());
  pWordLists->back().first = id;
  while (getline(fh,line)) {
    pWordLists->back().second.insert(line);
  }
}

void SparseReordering::AddScores(
              const TranslationOption& topt,
               LexicalReorderingState::ReorderingType reoType,
               LexicalReorderingConfiguration::Direction direction,
               ScoreComponentCollection* scores) const 
{
}

} //namespace

