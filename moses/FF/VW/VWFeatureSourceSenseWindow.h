#pragma once

#include <string>
#include <algorithm>
#include <boost/foreach.hpp>
#include "ThreadLocalByFeatureStorage.h"
#include "VWFeatureSource.h"
#include "moses/Util.h"

/*
 * Produces features from factors in the following format:
 * wordsense1:0.25^wordsense1:0.7^wordsense3:0.05
 *
 * This is useful e.g. for including different possible word senses as features weighted
 * by their probability.
 *
 * By default, features are extracted from a small context window around the current
 * phrase and from within the phrase.
 */

namespace Moses
{

class VWFeatureSourceSenseWindow : public VWFeatureSource
{
public:
  VWFeatureSourceSenseWindow(const std::string &line)
    : VWFeatureSource(line), m_tlsSenses(this), m_tlsForms(this), m_lexicalized(true), m_size(DEFAULT_WINDOW_SIZE) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  // precompute feature strings for each input sentence
  virtual void InitializeForInput(ttasksptr const& ttask) {
    InputType const& input = *(ttask->GetSource().get());

    std::vector<WordSenses>& senses = *m_tlsSenses.GetStored();
    std::vector<std::string>& forms = *m_tlsForms.GetStored();
    senses.clear();
    forms.clear();

    senses.resize(input.GetSize());
    forms.resize(input.GetSize());

    for (size_t i = 0; i < input.GetSize(); i++) {
      senses[i] = GetSenses(input, i);
      forms[i] = m_lexicalized ? GetWordForm(input, i) + "^" : "";
    }
  }

  void operator()(const InputType &input
                  , const Range &sourceRange
                  , Discriminative::Classifier &classifier
                  , Discriminative::FeatureVector &outFeatures) const {
    int begin = sourceRange.GetStartPos();
    int end   = sourceRange.GetEndPos() + 1;
    int inputLen = input.GetSize();

    const std::vector<WordSenses>& senses = *m_tlsSenses.GetStored();
    const std::vector<std::string>& forms = *m_tlsForms.GetStored();

    // before current phrase
    for (int i = std::max(0, begin - m_size); i < begin; i++) {
      BOOST_FOREACH(const Sense &sense, senses[i]) {
        outFeatures.push_back(classifier.AddLabelIndependentFeature("snsb^" + forms[i] + SPrint(i - begin) + "^" + sense.m_label, sense.m_prob));
        outFeatures.push_back(classifier.AddLabelIndependentFeature("snsb^" + forms[i] + sense.m_label, sense.m_prob));
      }
    }

    // within current phrase
    for (int i = begin; i < end; i++) {
      BOOST_FOREACH(const Sense &sense, senses[i]) {
        outFeatures.push_back(classifier.AddLabelIndependentFeature("snsin^" + forms[i] + SPrint(i - begin) + "^" + sense.m_label, sense.m_prob));
        outFeatures.push_back(classifier.AddLabelIndependentFeature("snsin^" + forms[i] + sense.m_label, sense.m_prob));
      }
    }

    // after current phrase
    for (int i = end; i < std::min(end + m_size, inputLen); i++) {
      BOOST_FOREACH(const Sense &sense, senses[i]) {
        outFeatures.push_back(classifier.AddLabelIndependentFeature("snsa^" + forms[i] + SPrint(i - begin) + "^" + sense.m_label, sense.m_prob));
        outFeatures.push_back(classifier.AddLabelIndependentFeature("snsa^" + forms[i] + sense.m_label, sense.m_prob));
      }
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    if (key == "size") {
      m_size = Scan<size_t>(value);
    } else if (key == "lexicalized") {
      m_lexicalized = Scan<bool>(value);
    } else {
      VWFeatureSource::SetParameter(key, value);
    }
  }

private:
  static const int DEFAULT_WINDOW_SIZE = 3;

  struct Sense {
    std::string m_label;
    float m_prob;
  };

  typedef std::vector<Sense> WordSenses;
  typedef ThreadLocalByFeatureStorage<std::vector<WordSenses> > TLSSenses;
  typedef ThreadLocalByFeatureStorage<std::vector<std::string> > TLSWordForms;

  TLSSenses m_tlsSenses; // for each input sentence, contains extracted senses and probs for each word
  TLSWordForms m_tlsForms; // word forms for each input sentence


  std::vector<Sense> GetSenses(const InputType &input, size_t pos) const {
    std::string w = GetWord(input, pos);
    std::vector<std::string> senseTokens = Tokenize(w, "^");

    std::vector<Sense> out(senseTokens.size());
    for (size_t i = 0; i < senseTokens.size(); i++) {
      std::vector<std::string> senseColumns = Tokenize(senseTokens[i], ":");
      if (senseColumns.size() != 2) {
        UTIL_THROW2("VW :: bad format of sense distribution: " << senseTokens[i]);
      }
      out[i].m_label = senseColumns[0];
      out[i].m_prob = Scan<float>(senseColumns[1]);
    }

    return out;
  }

  // assuming that word surface form is always factor 0, output the word form
  inline std::string GetWordForm(const InputType &input, size_t pos) const {
    return input.GetWord(pos).GetString(0).as_string();
  }

  bool m_lexicalized;
  int m_size;
};

}
