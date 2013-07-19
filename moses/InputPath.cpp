#include "InputPath.h"
#include "ScoreComponentCollection.h"
#include "TargetPhraseCollection.h"
#include "StaticData.h"
#include "TypeDef.h"

namespace Moses
{
InputPath::InputPath(const Phrase &phrase, const WordsRange &range, const InputPath *prevNode
                     ,const ScoreComponentCollection *inputScore)
  :m_prevNode(prevNode)
  ,m_phrase(phrase)
  ,m_range(range)
  ,m_inputScore(inputScore)
{
  FactorType factorType = StaticData::Instance().GetPlaceholderFactor();
  if (factorType != NOT_FOUND) {
    for (size_t pos = 0; pos < m_phrase.GetSize(); ++pos) {
      if (m_phrase.GetFactor(pos, factorType)) {
        m_placeholders.push_back(pos);
      }
    }
  }
}

InputPath::~InputPath()
{
  delete m_inputScore;

  // detach target phrase before delete objects from m_copiedSet
  // the phrase dictionary owns the target phrases so they should be doing the deleting
  std::vector<TargetPhraseCollection>::iterator iter;
  for (iter = m_copiedSet.begin(); iter != m_copiedSet.end(); ++iter) {
    TargetPhraseCollection &coll = *iter;
    coll.Detach();
  }
}

const TargetPhraseCollection *InputPath::GetTargetPhrases(const PhraseDictionary &phraseDictionary) const
{
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  iter = m_targetPhrases.find(&phraseDictionary);
  if (iter == m_targetPhrases.end()) {
    return NULL;
  }
  return iter->second.first;
}

const void *InputPath::GetPtNode(const PhraseDictionary &phraseDictionary) const
{
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  iter = m_targetPhrases.find(&phraseDictionary);
  if (iter == m_targetPhrases.end()) {
    return NULL;
  }
  return iter->second.second;
}

void InputPath::SetTargetPhrases(const PhraseDictionary &phraseDictionary
                                 , const TargetPhraseCollection *targetPhrases
                                 , const void *ptNode)
{
  std::pair<const TargetPhraseCollection*, const void*> value(targetPhrases, ptNode);
  m_targetPhrases[&phraseDictionary] = value;
}

std::ostream& operator<<(std::ostream& out, const InputPath& obj)
{
  out << &obj << " " << obj.GetWordsRange() << " " << obj.GetPrevNode() << " " << obj.GetPhrase();

  out << "pt: ";
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  for (iter = obj.m_targetPhrases.begin(); iter != obj.m_targetPhrases.end(); ++iter) {
    const PhraseDictionary *pt = iter->first;
    out << pt << " ";
  }

  return out;
}

}
