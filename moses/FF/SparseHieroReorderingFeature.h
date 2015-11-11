#pragma once

#include <string>

#include <boost/unordered_set.hpp>

#include <util/string_piece.hh>

#include "moses/Factor.h"
#include "moses/Sentence.h"

#include "StatelessFeatureFunction.h"

namespace Moses
{

class SparseHieroReorderingFeature : public StatelessFeatureFunction
{
public:
  enum Type {
    SourceCombined,
    SourceLeft,
    SourceRight
  };

  SparseHieroReorderingFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedScores) const {
  }
  virtual void EvaluateWithSourceContext(const InputType &input
                                         , const InputPath &inputPath
                                         , const TargetPhrase &targetPhrase
                                         , const StackVec *stackVec
                                         , ScoreComponentCollection &scoreBreakdown
                                         , ScoreComponentCollection *estimatedScores = NULL)  const {
  }

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  virtual void EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const;


private:

  typedef boost::unordered_set<const Factor*> Vocab;

  void AddNonTerminalPairFeatures(
    const Sentence& sentence, const Range& nt1, const Range& nt2,
    bool isMonotone, ScoreComponentCollection* accumulator) const;

  void LoadVocabulary(const std::string& filename, Vocab& vocab);
  const Factor*  GetFactor(const Word& word, const Vocab& vocab, FactorType factor) const;

  Type m_type;
  FactorType m_sourceFactor;
  FactorType m_targetFactor;
  std::string m_sourceVocabFile;
  std::string m_targetVocabFile;

  const Factor* m_otherFactor;

  Vocab m_sourceVocab;
  Vocab m_targetVocab;

};


}

