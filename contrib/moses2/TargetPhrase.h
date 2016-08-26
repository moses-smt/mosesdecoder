/*
 * TargetPhrase.h
 *
 *  Created on: 26 Apr 2016
 *      Author: hieu
 */

#pragma once
#include <sstream>
#include "PhraseImplTemplate.h"
#include "System.h"
#include "Scores.h"
#include "AlignmentInfoCollection.h"

namespace Moses2
{
class AlignmentInfo;

template<typename WORD>
class TargetPhrase: public PhraseImplTemplate<WORD>
{
public:
  typedef PhraseImplTemplate<WORD> Parent;
  const PhraseTable &pt;
  mutable void **ffData;
  SCORE *scoreProperties;

  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system, size_t size)
  : PhraseImplTemplate<WORD>(pool, size)
  , pt(pt)
  , scoreProperties(NULL)
  , m_alignTerm(&AlignmentInfoCollection::Instance().GetEmptyAlignmentInfo())
  {
    m_scores = new (pool.Allocate<Scores>()) Scores(system, pool,
      system.featureFunctions.GetNumScores());
  }

  Scores &GetScores()
  {  return *m_scores; }

  const Scores &GetScores() const
  {  return *m_scores; }

  virtual SCORE GetScoreForPruning() const = 0;

  SCORE *GetScoresProperty(int propertyInd) const
  {    return scoreProperties ? scoreProperties + propertyInd : NULL; }

  const AlignmentInfo &GetAlignTerm() const {
    return *m_alignTerm;
  }

  void SetAlignTerm(const AlignmentInfo &alignInfo) {
    m_alignTerm = &alignInfo;
  }

  void OutputToStream(const Hypothesis &hypo, std::ostream &out) const
  {
	size_t size = PhraseImplTemplate<WORD>::GetSize();
	if (size) {
	  (*this)[0].OutputToStream(out);
	  for (size_t i = 1; i < size; ++i) {
		const WORD &word = (*this)[i];
		out << " BOO:";
		word.OutputToStream(out);
	  }
	}
  }

  virtual std::string Debug(const System &system) const
  {
    std::stringstream out;
    out << Phrase<WORD>::Debug(system);
    out << " SCORES:" << GetScores().Debug(system);

    return out.str();
  }

protected:
  Scores *m_scores;
  const AlignmentInfo *m_alignTerm;
};

///////////////////////////////////////////////////////////////////////
template<typename TP>
struct CompareScoreForPruning
{
  bool operator()(const TP *a, const TP *b) const
  {
    return a->GetScoreForPruning() > b->GetScoreForPruning();
  }

  bool operator()(const TP &a, const TP &b) const
  {
    return a.GetScoreForPruning() > b.GetScoreForPruning();
  }
};

} /* namespace Moses2a */

