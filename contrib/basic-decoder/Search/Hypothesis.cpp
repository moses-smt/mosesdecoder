
#include <boost/functional/hash.hpp>
#include "Hypothesis.h"
#include "TargetPhrase.h"
#include "Sentence.h"
#include "WordsRange.h"
#include "Util.h"
#include "FF/StatefulFeatureFunction.h"

using namespace std;

size_t Hypothesis::s_id = 0;

Hypothesis::Hypothesis(const TargetPhrase &tp, const WordsRange &range, const WordsBitmap &coverage)
  :m_id(++s_id)
  ,targetPhrase(tp)
  ,m_range(range)
  ,m_prevHypo(NULL)
  ,m_coverage(coverage)
  ,m_scores(tp.GetScores())
  ,m_hash(0)
  ,targetRange(NOT_FOUND, NOT_FOUND)
{
  size_t numSFF = StatefulFeatureFunction::GetColl().size();
  m_ffStates.resize(numSFF);
}

Hypothesis::Hypothesis(const TargetPhrase &tp, const Hypothesis &prevHypo, const WordsRange &range, const WordsBitmap &coverage)
  :m_id(++s_id)
  ,targetPhrase(tp)
  ,m_range(range)
  ,m_prevHypo(&prevHypo)
  ,m_coverage(coverage)
  ,m_scores(prevHypo.GetScores())
  ,m_hash(0)
  ,targetRange(prevHypo.targetRange, tp.GetSize())
{
  m_scores.Add(targetPhrase.GetScores());
  size_t numSFF = StatefulFeatureFunction::GetColl().size();
  m_ffStates.resize(numSFF);
}

Hypothesis::~Hypothesis()
{
}

size_t Hypothesis::GetHash() const
{
  if (m_hash == 0) {
    // do nothing, assume already hashed
    // m_hash can be 0, but very small prob, or no statefull ff
    size_t numStates = StatefulFeatureFunction::GetColl().size();
    for (size_t i = 0; i < numStates; ++i) {
      size_t state = m_ffStates[i];
      boost::hash_combine(m_hash, state);
    }
  }

  return m_hash;
}

bool Hypothesis::operator==(const Hypothesis &other) const
{
  size_t numStates = StatefulFeatureFunction::GetColl().size();
  for (size_t i = 0; i < numStates; ++i) {
    size_t state = m_ffStates[i];
    size_t otherState = other.m_ffStates[i];

    bool isEqual = (state == otherState);
    if (!isEqual) {
      return false;
    }
  }

  return true;
}

void Hypothesis::Output(std::ostream &out) const
{
  if (m_prevHypo) {
    m_prevHypo->Output(out);
  }
  targetPhrase.Output(out);
}

std::string Hypothesis::Debug() const
{
  stringstream strme;
  Fix(strme, 3);
  strme << m_range.Debug() << " targetRange=" << targetRange.Debug() << " " << m_scores.Debug() << " ";
  Output(strme);

  // states
  strme << "states=";
  size_t numSFF = StatefulFeatureFunction::GetColl().size();
  for (size_t i = 0; i < numSFF; ++i) {
    size_t state = m_ffStates[i];
    strme << state << ",";
  }
  strme << "=" << m_hash;

  /*
    if (m_prevHypo) {
  	  strme << endl;
  	  strme << m_prevHypo->Debug();
    }
   */
  return strme.str();
}

const Word &Hypothesis::GetWord(size_t pos) const
{
  assert(pos <= targetRange.endPos);
  const Hypothesis *hypo = this;
  while (pos < hypo->targetRange.startPos) {
    hypo = hypo->GetPrevHypo();
    assert(hypo != NULL);
  }
  return hypo->GetCurrWord(pos - hypo->targetRange.startPos);
}

