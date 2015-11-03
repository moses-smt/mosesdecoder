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

  const Moses::Bitmap &GetBitmap() const
  { return m_sourceCompleted; }

  const Moses::Range &GetRange() const
  { return m_range; }

  const Scores &GetScores() const
  { return *m_scores; }

  const TargetPhrase &GetTargetPhrase() const
  { return m_targetPhrase; }

  const Moses::FFState *GetState(size_t ind) const
  { return m_ffStates[ind]; }

  void OutputToStream(std::ostream &out) const;

  void EmptyHypothesisState(const PhraseImpl &input);

  void EvaluateWhenApplied();

protected:
  Manager &m_mgr;
  const TargetPhrase &m_targetPhrase;
  const Moses::Bitmap &m_sourceCompleted;
  const Moses::Range &m_range;
  const Hypothesis *m_prevHypo;

  const Moses::FFState **m_ffStates;
  Scores *m_scores;
};


class HypothesisScoreOrderer
{
public:
  bool operator()(const Hypothesis* a, const Hypothesis* b) const {
    return a->GetScores().GetTotalScore() > b->GetScores().GetTotalScore();
  }
};

#endif /* HYPOTHESIS_H_ */
