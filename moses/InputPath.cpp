#include "InputPath.h"
#include "ScoreComponentCollection.h"
#include "TargetPhraseCollection.h"
#include "StaticData.h"
#include "TypeDef.h"
#include "AlignmentInfo.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
InputPath::
InputPath(const Phrase &phrase, const NonTerminalSet &sourceNonTerms,
          const WordsRange &range, const InputPath *prevNode,
          const ScorePair *inputScore)
  :m_prevPath(prevNode)
  ,m_phrase(phrase)
  ,m_range(range)
  ,m_inputScore(inputScore)
  ,m_sourceNonTerms(sourceNonTerms)
  ,m_nextNode(1)
{
  //cerr << "phrase=" << phrase << " m_inputScore=" << *m_inputScore << endl;

}

InputPath::~InputPath()
{
  delete m_inputScore;
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

const Word &InputPath::GetLastWord() const
{
  size_t len = m_phrase.GetSize();
  UTIL_THROW_IF2(len == 0, "Input path phrase cannot be empty");
  const Word &ret = m_phrase.GetWord(len - 1);
  return ret;
}

std::ostream& operator<<(std::ostream& out, const InputPath& obj)
{
  out << &obj << " " << obj.GetWordsRange() << " " << obj.GetPrevPath() << " " << obj.GetPhrase();

  out << "pt: ";
  std::map<const PhraseDictionary*, std::pair<const TargetPhraseCollection*, const void*> >::const_iterator iter;
  for (iter = obj.m_targetPhrases.begin(); iter != obj.m_targetPhrases.end(); ++iter) {
    const PhraseDictionary *pt = iter->first;
    out << pt << " ";
  }

  return out;
}

}
