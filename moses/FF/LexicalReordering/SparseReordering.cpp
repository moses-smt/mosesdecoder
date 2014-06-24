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

const std::string& SparseReorderingFeatureKey::Name(const string& wordListId) {
  static string kSep = "-";
  static string name;
  ostringstream buf;
  // type side position id word reotype
  if (type == Phrase) {
    buf << "phr";
  } else if (type == Stack) {
    buf << "stk";
  } else if (type == Between) {
    buf << "btn";
  }
  buf << kSep;
  if (side == Source) {
    buf << "src";
  } else if (side == Target) {
    buf << "tgt";
  }
  buf << kSep;
  if (position == First) {
    buf << "first";
  } else if (position == Last) {
    buf << "last";
  }
  buf << kSep;
  buf << wordListId;
  buf << kSep;
  buf << word->GetString();
  buf << kSep;
  buf << reoType;
  name = buf.str();
  return name;
}

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
        ReadWordList(i->second,fields[2], SparseReorderingFeatureKey::Source, &m_sourceWordLists);
      } else if (fields[1] == kTarget) {
        ReadWordList(i->second,fields[2],SparseReorderingFeatureKey::Target, &m_targetWordLists);
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

void SparseReordering::ReadWordList(const string& filename, const string& id, SparseReorderingFeatureKey::Side side, vector<WordList>* pWordLists) {
  ifstream fh(filename.c_str());
  UTIL_THROW_IF(!fh, util::Exception, "Unable to open: " << filename);
  string line;
  pWordLists->push_back(WordList());
  pWordLists->back().first = id;
  while (getline(fh,line)) {
    //TODO: StringPiece
    const Factor* factor = FactorCollection::Instance().AddFactor(line);
    pWordLists->back().second.insert(factor);
    //Pre-calculate feature names.
    for (size_t type = SparseReorderingFeatureKey::Stack;
                       type <= SparseReorderingFeatureKey::Between; ++type) {
      for (size_t position = SparseReorderingFeatureKey::First;
                       position <= SparseReorderingFeatureKey::Last; ++position) {
        for (int reoType = 0; reoType <= LexicalReorderingState::MAX; ++reoType) {
          SparseReorderingFeatureKey key(
            pWordLists->size()-1, static_cast<SparseReorderingFeatureKey::Type>(type),
            factor, static_cast<SparseReorderingFeatureKey::Position>(position), side, reoType);
          m_featureMap[key] = key.Name(id);
        }
      }
    }

  }
}

void SparseReordering::AddFeatures(size_t id,
    SparseReorderingFeatureKey::Type type, SparseReorderingFeatureKey::Side side,
    const Word& word, SparseReorderingFeatureKey::Position position,
    const WordList& words, LexicalReorderingState::ReorderingType reoType,
    ScoreComponentCollection* scores) const {

  const Factor*  wordFactor = word.GetFactor(0);
  if (words.second.find(wordFactor) == words.second.end()) return;
  SparseReorderingFeatureKey key(id, type, wordFactor, position, side, reoType);
  FeatureMap::const_iterator fmi = m_featureMap.find(key);
  assert(fmi != m_featureMap.end());
  scores->PlusEquals(m_producer, fmi->second, 1.0);

}

void SparseReordering::CopyScores(
               const TranslationOption& currentOpt,
               const TranslationOption* previousOpt,
               const InputType& input,
               LexicalReorderingState::ReorderingType reoType,
               LexicalReorderingConfiguration::Direction direction,
               ScoreComponentCollection* scores) const 
{
  if (m_useBetween && direction == LexicalReorderingConfiguration::Backward &&
      (reoType == LexicalReorderingState::D || reoType == LexicalReorderingState::DL ||
        reoType == LexicalReorderingState::DR)) {
    size_t gapStart, gapEnd;
    const Sentence& sentence = dynamic_cast<const Sentence&>(input);
    const WordsRange& currentRange = currentOpt.GetSourceWordsRange();
    if (previousOpt) {
      const WordsRange& previousRange = previousOpt->GetSourceWordsRange();
      if (previousRange < currentRange) {
        gapStart = previousRange.GetEndPos() + 1;
        gapEnd = currentRange.GetStartPos();
      } else {
        gapStart = currentRange.GetEndPos() + 1;
        gapEnd = previousRange.GetStartPos();
      }
    } else {
      //start of sentence
      gapStart = 0;
      gapEnd  = currentRange.GetStartPos();
    }
    assert(gapStart < gapEnd);
    for (size_t i = gapStart; i < gapEnd; ++i) {
      for (size_t j = 0; j < m_sourceWordLists.size(); ++j) {
        AddFeatures(j, SparseReorderingFeatureKey::Between,
           SparseReorderingFeatureKey::Source, sentence.GetWord(i),
          SparseReorderingFeatureKey::First, m_sourceWordLists[j], reoType, scores);
      }
    }
  }
  //std::cerr << "SR " << topt << " " << reoType << " " << direction << std::endl;
  //phrase (backward)
  //stack (forward)
  SparseReorderingFeatureKey::Type type;
  if (direction == LexicalReorderingConfiguration::Forward) {
    if (!m_useStack) return;
    type = SparseReorderingFeatureKey::Stack;
  } else if (direction == LexicalReorderingConfiguration::Backward) {
    if (!m_usePhrase) return;
    type = SparseReorderingFeatureKey::Phrase;
  } else {
    //Shouldn't be called for bidirectional
    //keep compiler happy
    type = SparseReorderingFeatureKey::Phrase;
    assert(!"Shouldn't call CopyScores() with bidirectional direction");
  }
  for (size_t i = 0; i < m_sourceWordLists.size(); ++i) {
    const Phrase& sourcePhrase = currentOpt.GetInputPath().GetPhrase();
    AddFeatures(i, type, SparseReorderingFeatureKey::Source, sourcePhrase.GetWord(0),
      SparseReorderingFeatureKey::First, m_sourceWordLists[i], reoType, scores);
    AddFeatures(i, type, SparseReorderingFeatureKey::Source, sourcePhrase.GetWord(sourcePhrase.GetSize()-1),
      SparseReorderingFeatureKey::Last, m_sourceWordLists[i], reoType, scores);
  }
  for (size_t i = 0; i < m_targetWordLists.size(); ++i) {
    const Phrase& targetPhrase = currentOpt.GetTargetPhrase();   
    AddFeatures(i, type, SparseReorderingFeatureKey::Target, targetPhrase.GetWord(0),
      SparseReorderingFeatureKey::First, m_targetWordLists[i], reoType, scores);
    AddFeatures(i, type, SparseReorderingFeatureKey::Target, targetPhrase.GetWord(targetPhrase.GetSize()-1),
      SparseReorderingFeatureKey::Last, m_targetWordLists[i], reoType, scores);
  }


}

} //namespace

