/*
 * Hypothesis.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef HYPOTHESIS_H_
#define HYPOTHESIS_H_

#include <iostream>
#include <cstddef>
#include "moses/FF/FFState.h"
#include "moses/Bitmap.h"
#include "../Scores.h"
#include "../TargetPhrase.h"

class Manager;
class PhraseImpl;
class TargetPhrase;
class Scores;

class Hypothesis {
	  friend std::ostream& operator<<(std::ostream &, const Hypothesis &);
public:
  Hypothesis(Manager &mgr,
		  const TargetPhrase &tp,
		  const Moses::Range &range,
		  const Moses::Bitmap &bitmap);
  Hypothesis(const Hypothesis &prevHypo,
		  const TargetPhrase &tp,
		  const Moses::Range &pathRange,
		  const Moses::Bitmap &bitmap);
  virtual ~Hypothesis();

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

  inline const Moses::Bitmap &GetBitmap() const
  { return m_sourceCompleted; }

  inline const Moses::Range &GetRange() const
  { return m_range; }

  inline const Moses::Range &GetCurrTargetWordsRange() const {
    return m_currTargetWordsRange;
  }

  const Scores &GetScores() const
  { return *m_scores; }

  const TargetPhrase &GetTargetPhrase() const
  { return m_targetPhrase; }

  const Moses::FFState *GetState(size_t ind) const
  { return m_ffStates[ind]; }

  void OutputToStream(std::ostream &out) const;

  void EmptyHypothesisState(const PhraseImpl &input);

  void EvaluateWhenApplied();

  const Hypothesis* GetPrevHypo() const
  { return m_prevHypo; }

  /** curr - pos is relative from CURRENT hypothesis's starting index
   * (ie, start of sentence would be some negative number, which is
   * not allowed- USE WITH CAUTION) */
  inline const Word &GetCurrWord(size_t pos) const {
    return m_targetPhrase[pos];
  }

  /** recursive - pos is relative from start of sentence */
  const Word &GetWord(size_t pos) const;

protected:
  Manager &m_mgr;
  const TargetPhrase &m_targetPhrase;
  const Moses::Bitmap &m_sourceCompleted;
  const Moses::Range &m_range;
  const Hypothesis *m_prevHypo;

  const Moses::FFState **m_ffStates;
  Scores *m_scores;
  Moses::Range m_currTargetWordsRange;
};


class HypothesisScoreOrderer
{
public:
  bool operator()(const Hypothesis* a, const Hypothesis* b) const {
    return a->GetScores().GetTotalScore() > b->GetScores().GetTotalScore();
  }
};

#endif /* HYPOTHESIS_H_ */
