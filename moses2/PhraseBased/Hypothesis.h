/*
 * Hypothesis.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <iostream>
#include <cstddef>
#include "../FF/FFState.h"
#include "../legacy/Bitmap.h"
#include "../legacy/Range.h"
#include "../Scores.h"
#include "../Phrase.h"
#include "../TargetPhrase.h"
#include "../InputPathBase.h"
#include "../HypothesisBase.h"

namespace Moses2
{
class Manager;
class InputType;
class StatefulFeatureFunction;
class TargetPhraseImpl;

class Hypothesis: public HypothesisBase
{
  Hypothesis(MemPool &pool, const System &system);

public:

  static Hypothesis *Create(MemPool &pool, Manager &mgr);
  virtual ~Hypothesis();

  // initial, empty hypo
  void Init(Manager &mgr, const InputPathBase &path, const TargetPhraseImpl &tp,
            const Bitmap &bitmap);

  void Init(Manager &mgr, const Hypothesis &prevHypo, const InputPathBase &path,
            const TargetPhraseImpl &tp, const Bitmap &bitmap, SCORE estimatedScore);

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

  inline const Bitmap &GetBitmap() const {
    return *m_sourceCompleted;
  }

  inline const InputPathBase &GetInputPath() const {
    return *m_path;
  }

  inline const Range &GetCurrTargetWordsRange() const {
    return m_currTargetWordsRange;
  }

  SCORE GetFutureScore() const {
    return GetScores().GetTotalScore() + m_estimatedScore;
  }

  const TargetPhrase<Moses2::Word> &GetTargetPhrase() const {
    return *m_targetPhrase;
  }

  std::string Debug(const System &system) const;

  virtual void OutputToStream(std::ostream &out) const;

  void EmptyHypothesisState(const InputType &input);

  void EvaluateWhenApplied();
  void EvaluateWhenApplied(const StatefulFeatureFunction &sfff);

  const Hypothesis* GetPrevHypo() const {
    return m_prevHypo;
  }

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
  const TargetPhrase<Moses2::Word> *m_targetPhrase;
  const Bitmap *m_sourceCompleted;
  const InputPathBase *m_path;
  const Hypothesis *m_prevHypo;

  SCORE m_estimatedScore;
  Range m_currTargetWordsRange;
};

////////////////////////////////////////////////////////////////////////////////////
class HypothesisTargetPhraseOrderer
{
public:
  bool operator()(const Hypothesis* a, const Hypothesis* b) const {
    PhraseOrdererLexical<Moses2::Word> phraseCmp;
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

