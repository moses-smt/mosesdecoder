#include <fstream>

#include "moses/FactorCollection.h"
#include "moses/InputPath.h"
#include "moses/Util.h"
#include "util/exception.hh"

#include "LexicalReordering.h"
#include "SparseReordering.h"


using namespace std;

namespace Moses 
{

SparseReordering::SparseReordering(const map<string,string>& config, const LexicalReordering* producer)
  : m_producer(producer) 
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
  UTIL_THROW_IF(!fh, util::Exception, "Unable to open: " << filename);
  string line;
  pWordLists->push_back(WordList());
  pWordLists->back().first = id;
  while (getline(fh,line)) {
    //TODO: StringPiece
    const Factor* factor = FactorCollection::Instance().AddFactor(line);
    pWordLists->back().second.insert(factor);
  }
}

void SparseReordering::AddFeatures(
    const string& type, const Word& word, const string& position,  const WordList& words,
    LexicalReorderingState::ReorderingType reoType,
    ScoreComponentCollection* scores) const {

  //TODO: Precalculate all feature names
  static string kSep = "-";
  const Factor*  wordFactor = word.GetFactor(0);
  if (words.second.find(wordFactor) == words.second.end()) return;
  ostringstream buf;
  buf  << type << kSep << position << kSep << words.first << kSep << wordFactor->GetString() << kSep << reoType;
  scores->PlusEquals(m_producer, buf.str(), 1.0);

}

void SparseReordering::CopyScores(
               const TranslationOption& topt,
               LexicalReorderingState::ReorderingType reoType,
               LexicalReorderingConfiguration::Direction direction,
               ScoreComponentCollection* scores) const 
{
  //std::cerr << "SR " << topt << " " << reoType << " " << direction << std::endl;
  const string kPhrase = "phr"; //phrase (backward)
  const string kStack = "stk"; //stack (forward)

  const string* type = &kPhrase;
  //TODO: bidirectional?
  if (direction == LexicalReorderingConfiguration::Forward) {
    if (!m_useStack) return;
    type = &kStack;
  } else if (direction == LexicalReorderingConfiguration::Backward && !m_usePhrase) {
    return;
  }
  for (vector<WordList>::const_iterator i = m_sourceWordLists.begin(); i != m_sourceWordLists.end(); ++i) {
    const Phrase& sourcePhrase = topt.GetInputPath().GetPhrase();
    AddFeatures(*type, sourcePhrase.GetWord(0), "src.first", *i, reoType, scores);
    AddFeatures(*type, sourcePhrase.GetWord(sourcePhrase.GetSize()-1), "src.last", *i, reoType, scores);
  }
  for (vector<WordList>::const_iterator i = m_targetWordLists.begin(); i != m_targetWordLists.end(); ++i) {
    const Phrase& targetPhrase = topt.GetTargetPhrase();   
    AddFeatures(*type, targetPhrase.GetWord(0), "tgt.first", *i, reoType, scores);
    AddFeatures(*type, targetPhrase.GetWord(targetPhrase.GetSize()-1), "tgt.last", *i, reoType, scores);
  }


}

} //namespace

