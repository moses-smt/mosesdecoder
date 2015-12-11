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
#include "../legacy/FFState.h"
#include "../legacy/Bitmap.h"
#include "../Scores.h"
#include "../TargetPhrase.h"
#include "../legacy/Range.h"

class Manager;
class PhraseImpl;
class TargetPhrase;
class Scores;
class StatefulFeatureFunction;

class Hypothesis {
	  friend std::ostream& operator<<(std::ostream &, const Hypothesis &);

	  Hypothesis(Manager &mgr);

public:
  Manager &mgr;

  static Hypothesis *Create(Manager &mgr);
  virtual ~Hypothesis();

  // initial, empty hypo
  void Init(const TargetPhrase &tp,
  		const Range &range,
  		const Bitmap &bitmap);

  void Init(const Hypothesis &prevHypo,
  		const TargetPhrase &tp,
  		const Range &pathRange,
  		const Bitmap &bitmap,
		SCORE estimatedScore);

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

  inline const Bitmap &GetBitmap() const
  { return *m_sourceCompleted; }

  inline const Range &GetRange() const
  { return *m_range; }

  inline const Range &GetCurrTargetWordsRange() const {
    return m_currTargetWordsRange;
  }

  const Scores &GetScores() const
  { return *m_scores; }

  SCORE GetFutureScore() const
  { return GetScores().GetTotalScore() + m_estimatedScore; }

  const TargetPhrase &GetTargetPhrase() const
  { return *m_targetPhrase; }

  const FFState *GetState(size_t ind) const
  { return m_ffStates[ind]; }

  void OutputToStream(std::ostream &out) const;

  void EmptyHypothesisState(const PhraseImpl &input);

  /** Only evaluates non-batched stateful FFs. */
  void EvaluateWhenApplied();
  void EvaluateWhenApplied(const StatefulFeatureFunction &sfff);

  /** Hieu's old failed batching. Has nothing to do with BatchedFeatureFunction. */
  void EvaluateWhenAppliedNonBatch();

  const Hypothesis* GetPrevHypo() const
  { return m_prevHypo; }

  /** curr - pos is relative from CURRENT hypothesis's starting index
   * (ie, start of sentence would be some negative number, which is
   * not allowed- USE WITH CAUTION) */
  inline const Word &GetCurrWord(size_t pos) const {
    return GetTargetPhrase()[pos];
  }

  /** recursive - pos is relative from start of sentence */
  const Word &GetWord(size_t pos) const;

  void Swap(Hypothesis &other);
protected:
  const TargetPhrase *m_targetPhrase;
  const Bitmap *m_sourceCompleted;
  const Range *m_range;
  const Hypothesis *m_prevHypo;

  FFState **m_ffStates;
  Scores *m_scores;
  SCORE m_estimatedScore;
  Range m_currTargetWordsRange;
};


class HypothesisFutureScoreOrderer
{
public:
  bool operator()(const Hypothesis* a, const Hypothesis* b) const {
    return a->GetFutureScore() > b->GetFutureScore();
  }
};

#endif /* HYPOTHESIS_H_ */
