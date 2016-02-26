/*
 * Hypothesis.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <iostream>
#include <cstddef>
#include "../legacy/FFState.h"
#include "../legacy/Bitmap.h"
#include "../Scores.h"
#include "../Phrase.h"
#include "../InputPath.h"
#include "../legacy/Range.h"

namespace Moses2
{

class Manager;
class PhraseImpl;
class TargetPhrase;
class Scores;
class StatefulFeatureFunction;
class InputType;
class InputPath;

class Hypothesis {
	  friend std::ostream& operator<<(std::ostream &, const Hypothesis &);

	  Hypothesis(MemPool &pool, const System &system);

public:

  static Hypothesis *Create(MemPool &pool, Manager &mgr);
  virtual ~Hypothesis();

  // initial, empty hypo
  void Init(Manager &mgr, const InputPath &path, const TargetPhrase &tp, const Bitmap &bitmap);

  void Init(Manager &mgr, const Hypothesis &prevHypo,
  	    const InputPath &path,
  		const TargetPhrase &tp,
  		const Bitmap &bitmap,
		SCORE estimatedScore);

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

  inline Manager &GetManager() const
  { return *m_mgr; }

  inline const Bitmap &GetBitmap() const
  { return *m_sourceCompleted; }

  inline const InputPath &GetInputPath() const
  { return *m_path; }

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

  void EmptyHypothesisState(const InputType &input);

  void EvaluateWhenApplied();
  void EvaluateWhenApplied(const StatefulFeatureFunction &sfff);
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
  Manager *m_mgr;
  const TargetPhrase *m_targetPhrase;
  const Bitmap *m_sourceCompleted;
  const InputPath *m_path;
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

class HypothesisTargetPhraseOrderer
{
public:
  bool operator()(const Hypothesis* a, const Hypothesis* b) const {
	PhraseOrdererLexical phraseCmp;
	bool ret = phraseCmp(a->GetTargetPhrase(), b->GetTargetPhrase());
	/*
	std::cerr << (const Phrase&) a->GetTargetPhrase() << " ||| "
			<< (const Phrase&) b->GetTargetPhrase() << " ||| "
			<< ret << std::endl;
	*/
    return ret;
  }
};

}

