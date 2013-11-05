#include <iostream>
#include <cassert>
#include "LM.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "MyVocab.h"
#include "Search/Hypothesis.h"

using namespace std;

namespace FastMoses
{

LM::LM(const std::string &line)
  :StatefulFeatureFunction(line)
{
  m_bos.CreateFromString("<s>");
  m_eos.CreateFromString("</s>");
}

void LM::Evaluate(const Phrase &source
                  , const TargetPhrase &targetPhrase
                  , Scores &scores
                  , Scores &estimatedFutureScore) const
{
  SCORE all = 0, ngram = 0;

  PhraseVec phraseVec;
  phraseVec.reserve(m_order);
  for (size_t pos = 0; pos < targetPhrase.GetSize(); ++pos) {
    const Word &word = targetPhrase.GetWord(pos);
    ShiftOrPush(phraseVec, word);
    SCORE score = GetValueCache(phraseVec);

    all += score;
    if (phraseVec.size() == m_order) {
      ngram += score;
    }
  }

  SCORE estimated = all - ngram;
  scores.Add(*this, ngram);
  estimatedFutureScore.Add(*this, estimated);
}

size_t LM::Evaluate(
  const Hypothesis& hypo,
  size_t prevState,
  Scores &scores) const
{
  if (m_order <= 1) {
    return 0; // not sure if returning NULL is correct
  }

  if (hypo.targetPhrase.GetSize() == 0) {
    return 0; // not sure if returning NULL is correct
  }

    PhraseVec m_phraseVec(m_order);

  const size_t currEndPos = hypo.targetRange.endPos;
  const size_t startPos = hypo.targetRange.startPos;

  size_t index = 0;
  for (int currPos = (int) startPos - (int) m_order + 1 ; currPos <= (int) startPos ; currPos++) {
    if (currPos >= 0)
      m_phraseVec[index++] = &hypo.GetWord(currPos);
    else {
      m_phraseVec[index++] = &m_bos;
    }
  }

  SCORE lmScore = GetValueCache(m_phraseVec);

  // main loop
  size_t endPos = std::min(startPos + m_order - 2
                           , currEndPos);
  for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++) {
    // shift all args down 1 place
    for (size_t i = 0 ; i < m_order - 1 ; i++)
      m_phraseVec[i] = m_phraseVec[i + 1];

    // add last factor
    m_phraseVec.back() = &hypo.GetWord(currPos);

    lmScore	+= GetValueCache(m_phraseVec);
  }

  // end of sentence
  if (hypo.GetCoverage().IsComplete()) {
    const size_t size = hypo.GetSize();
    m_phraseVec.back() = &m_eos;

    for (size_t i = 0 ; i < m_order - 1 ; i ++) {
      int currPos = (int)(size - m_order + i + 1);
      if (currPos < 0)
        m_phraseVec[i] = &m_bos;
      else
        m_phraseVec[i] = &hypo.GetWord((size_t)currPos);
    }
    lmScore += GetValueCache(m_phraseVec);
  } else {
    if (endPos < currEndPos) {
      //need to get the LM state (otherwise the last LM state is fine)
      for (size_t currPos = endPos+1; currPos <= currEndPos; currPos++) {
        for (size_t i = 0 ; i < m_order - 1 ; i++)
          m_phraseVec[i] = m_phraseVec[i + 1];
        m_phraseVec.back() = &hypo.GetWord(currPos);
      }
    }
  }

  size_t state = GetLastState();
  return state;
}

SCORE LM::GetValueCache(const PhraseVec &phraseVec) const
{
   SCORE score = GetValue(phraseVec);
   return score;
    
  size_t hash = 0;
  for (size_t i = 0; i < phraseVec.size(); ++i) {
    VOCABID vocabId = phraseVec[i]->GetVocab();
    boost::hash_combine(hash, vocabId);
  }
  
  Cache::const_iterator iter;
  iter = m_cache.find(hash);
  if (iter != m_cache.end()) {
    return iter->second;
  }
  else {
    SCORE score = GetValue(phraseVec);
    m_cache[hash] = score;
    return score;
  }
}
  
void LM::ShiftOrPush(PhraseVec &phraseVec, const Word &word) const
{
  if (phraseVec.size() < m_order) {
    phraseVec.push_back(&word);
  } else {
    // shift
    for (size_t currNGramOrder = 0 ; currNGramOrder < m_order - 1 ; currNGramOrder++) {
      phraseVec[currNGramOrder] = phraseVec[currNGramOrder + 1];
    }
    phraseVec[m_order - 1] = &word;
  }
}

void LM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "order") {
    m_order = Scan<size_t>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }

}

}

