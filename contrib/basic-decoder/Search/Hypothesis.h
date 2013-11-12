
#pragma once

#include <ostream>
#include "WordsBitmap.h"
#include "WordsRange.h"
#include "Scores.h"
#include "Word.h"
#include "TargetPhrase.h"

class FFState;
class Sentence;
class WordsRange;

class Hypothesis
{
public:
  const TargetPhrase &targetPhrase;
  const WordsRange targetRange;

  Hypothesis(); // do no implement
  Hypothesis(const Hypothesis &copy); // do not implement

  // creating the inital hypo
  Hypothesis(const TargetPhrase &tp, const WordsRange &range, const WordsBitmap &coverage);

  // for extending a previous hypo
  Hypothesis(const TargetPhrase &tp, const Hypothesis &prevHypo, const WordsRange &range, const WordsBitmap &coverage);
  virtual ~Hypothesis();

  const Scores &GetScores() const {
    return m_scores;
  }
  Scores &GetScores() {
    return m_scores;
  }

  const WordsBitmap &GetCoverage() const {
    return m_coverage;
  }
  const Hypothesis *GetPrevHypo() const {
    return m_prevHypo;
  }

  const WordsRange &GetRange() const {
    return m_range;
  }

  size_t GetState(size_t id) const {
    return m_ffStates[id];
  }
  void SetState(size_t id, size_t state) {
    m_ffStates[id] = state;
  }

  const Word &GetWord(size_t pos) const;
  inline const Word &GetCurrWord(size_t pos) const {
    return targetPhrase.GetWord(pos);
  }

  /** length of the partial translation (from the start of the sentence) */
  inline size_t GetSize() const {
    return targetRange.endPos + 1;
  }

  void Output(std::ostream &out) const;

  size_t GetHash() const;
  bool operator==(const Hypothesis &other) const;

  std::string Debug() const;
  
  static size_t GetNumHypothesesCreated() {
    return s_id;
  }
protected:
  static size_t s_id;
  size_t m_id;

  const WordsRange &m_range;
  const Hypothesis *m_prevHypo;
  const WordsBitmap m_coverage;
  Scores m_scores;

  std::vector<size_t> m_ffStates;
  mutable size_t m_hash;
};


struct HypothesisHasher {
  size_t operator()(const Hypothesis *hypo) const {
    return hypo->GetHash();
  }
};

struct HypothesisEqual {
  bool operator()(const Hypothesis *a, const Hypothesis *b) const {
    bool ret = *a == *b;
    return ret;
  }
};

struct HypothesisScoreOrderer {
  bool operator()(const Hypothesis* a, const Hypothesis* b) const {
    return a->GetScores().GetWeightedScore() > b->GetScores().GetWeightedScore();
  }
};
