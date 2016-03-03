#pragma once

#include <string>
#include "VWFeatureBase.h"
#include "moses/InputType.h"
#include "moses/TypeDef.h"

namespace Moses
{

// Inherit from this for source-dependent classifier features. They will
// automatically register with the classifier class named VW0 or one or more
// names specified by the used-by=name1,name2,... parameter.
//
// The classifier gets a full list by calling
// VWFeatureBase::GetTargetContextFeatures(GetScoreProducerDescription())


class VWFeatureContext : public VWFeatureBase
{
public:
  VWFeatureContext(const std::string &line)
    : VWFeatureBase(line, vwft_targetContext) {
  }

  // Gets its pure virtual functions from VWFeatureBase

  virtual void operator()(const InputType &input
                          , const InputPath &inputPath
                          , const TargetPhrase &targetPhrase
                          , Discriminative::Classifier &classifier) const {
  }

  virtual void operator()(const InputType &input
                          , const InputPath &inputPath
                          , const Range &sourceRange
                          , Discriminative::Classifier &classifier) const {
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    VWFeatureBase::SetParameter(key, value);
  }

protected:
  // Get word with the correct subset of factors as string. Because we're target
  // context features, we look at a limited number of words to the left of the
  // current translation. posFromEnd is interpreted like this:
  // 0 = last word of the hypothesis
  // 1 = next to last word
  // ...etc.
  //
  // We may have to go through more than one hypothesis when the context is long.
  inline std::string GetWord(const Hypothesis *hypo, size_t posFromEnd) const {
    while (hypo->GetCurrTargetPhrase().GetSize() <= posFromEnd) {
      posFromEnd -= hypo->GetCurrTargetPhrase().GetSize();
      hypo = hypo->GetPrevHypo();
      if (! hypo)
        return BOS_; // translation is not long enough
    }

    const Phrase &phrase = hypo->GetCurrTargetPhrase();
    return phrase.GetWord(phrase.GetSize() - posFromEnd - 1).GetString(m_targetFactors, false);
  }

  inline std::string GetWord(const Phrase &phrase, size_t posFromEnd) const {
    return phrase.GetWord(phrase.GetSize() - posFromEnd - 1).GetString(m_targetFactors, false);
  }
};

}
