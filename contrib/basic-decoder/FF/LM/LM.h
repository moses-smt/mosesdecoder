#pragma once

#include <boost/unordered_map.hpp>
#include "FF/StatefulFeatureFunction.h"
#include "TypeDef.h"
#include "Phrase.h"


namespace FastMoses
{

class LM : public StatefulFeatureFunction
{
public:
  LM(const std::string &line);

  virtual size_t GetLastState() const = 0;

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , Scores &scores
                        , Scores &estimatedFutureScore) const;

  virtual size_t Evaluate(
    const Hypothesis& hypo,
    size_t prevState,
    Scores &scores) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  size_t m_order;
  Word  m_bos, m_eos;

  typedef boost::unordered_map<size_t, SCORE> Cache;
  mutable Cache m_cache;

  virtual SCORE GetValue(const PhraseVec &phraseVec) const = 0;
  SCORE GetValueCache(const PhraseVec &phraseVec) const;
  void ShiftOrPush(PhraseVec &phraseVec, const Word &word) const;
};

}
