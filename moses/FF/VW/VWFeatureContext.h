#pragma once

#include <string>
#include <boost/foreach.hpp>
#include "VWFeatureBase.h"
#include "moses/InputType.h"
#include "moses/TypeDef.h"
#include "moses/Word.h"

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
  VWFeatureContext(const std::string &line, size_t contextSize)
    : VWFeatureBase(line, vwft_targetContext), m_contextSize(contextSize) {
  }

  // Gets its pure virtual functions from VWFeatureBase

  virtual void operator()(const InputType &input
                          , const TargetPhrase &targetPhrase
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const {
  }

  virtual void operator()(const InputType &input
                          , const Range &sourceRange
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const {
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    if (key == "size") {
      m_contextSize = Scan<size_t>(value);
    } else if (key == "factor-positions") {
      // factor positions: assuming a factor such as positional morphological tag, use this
      // option to select only certain positions; this assumes that only a single
      // target-side factor is defined
      Tokenize<size_t>(m_factorPositions, value, ",");
    } else {
      VWFeatureBase::SetParameter(key, value);
    }
  }

  size_t GetContextSize() {
    return m_contextSize;
  }

protected:
  // Get word with the correct subset of factors as string. Because we're target
  // context features, we look at a limited number of words to the left of the
  // current translation. posFromEnd is interpreted like this:
  // 0 = last word of the hypothesis
  // 1 = next to last word
  // ...etc.
  inline std::string GetWord(const Phrase &phrase, size_t posFromEnd) const {
    const Word &word = phrase.GetWord(phrase.GetSize() - posFromEnd - 1);
    if (m_factorPositions.empty()) {
      return word.GetString(m_targetFactors, false);
    } else {
      if (m_targetFactors.size() != 1)
        UTIL_THROW2("You can only use factor-positions when a single target-side factor is defined.");
      const std::string &fullFactor = word.GetFactor(m_targetFactors[0])->GetString().as_string();

      // corner cases: at sentence beginning/end, we don't have the correct factors set up
      // similarly for UNK
      if (fullFactor == BOS_ || fullFactor == EOS_ || fullFactor == UNKNOWN_FACTOR)
        return fullFactor;

      std::string subFactor(m_factorPositions.size(), 'x'); // initialize string with correct size and placeholder chars
      for (size_t i = 0; i < m_factorPositions.size(); i++)
        subFactor[i] = fullFactor[m_factorPositions[i]];

      return subFactor;
    }
  }

  // some target-context feature functions also look at the source
  inline std::string GetSourceWord(const InputType &input, size_t pos) const {
    return input.GetWord(pos).GetString(m_sourceFactors, false);
  }

  // get source words aligned to a particular context word
  std::vector<std::string> GetAlignedSourceWords(const Phrase &contextPhrase
      , const InputType &input
      , const AlignmentInfo &alignInfo
      , size_t posFromEnd) const {
    size_t idx = contextPhrase.GetSize() - posFromEnd - 1;
    std::set<size_t> alignedToTarget = alignInfo.GetAlignmentsForTarget(idx);
    std::vector<std::string> out;
    out.reserve(alignedToTarget.size());
    BOOST_FOREACH(size_t srcIdx, alignedToTarget) {
      out.push_back(GetSourceWord(input, srcIdx));
    }
    return out;
  }

  // required context size
  size_t m_contextSize;

  // factor positions: assuming a factor such as positional morphological tag, use this
  // option to select only certain positions
  std::vector<size_t> m_factorPositions;
};

}
