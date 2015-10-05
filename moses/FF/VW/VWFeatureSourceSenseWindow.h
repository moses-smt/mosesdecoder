#pragma once

#include <string>
#include <algorithm>
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
    : VWFeatureSource(line), m_size(DEFAULT_WINDOW_SIZE) {
    ReadParameters();

    // Call this last
    VWFeatureBase::UpdateRegister();
  }

  void operator()(const InputType &input
      , const InputPath &inputPath
      , const WordsRange &sourceRange
      , Discriminative::Classifier &classifier) const {
    int begin = sourceRange.GetStartPos();
    int end   = sourceRange.GetEndPos() + 1;
    int inputLen = input.GetSize();

    // before current phrase
    for (int i = std::max(0, begin - m_size); i < begin; i++) {
      std::vector<Sense> senses = GetSenses(input, i);
      for (int senseIdx = 0; senseIdx < senses.size(); senseIdx++) {
        classifier.AddLabelIndependentFeature("snsb^" + GetWordForm(input, i) + "^" + SPrint(i - begin) + "^" + senses[senseIdx].m_word, senses[senseIdx].m_prob);
        classifier.AddLabelIndependentFeature("snsb^" + GetWordForm(input, i) + "^" + senses[senseIdx].m_word, senses[senseIdx].m_prob);
      }
    }

    // within current phrase
    for (int i = begin; i < end; i++) {
      std::vector<Sense> senses = GetSenses(input, i);
      for (int senseIdx = 0; senseIdx < senses.size(); senseIdx++) {
        classifier.AddLabelIndependentFeature("snsin^" + GetWordForm(input, i) + "^" + SPrint(i) + "^" +senses[senseIdx].m_word, senses[senseIdx].m_prob);
        classifier.AddLabelIndependentFeature("snsin^" + GetWordForm(input, i) + "^" + senses[senseIdx].m_word, senses[senseIdx].m_prob);
      }
    }

    // after current phrase
    for (int i = end; i < std::min(end + m_size, inputLen); i++) {
      std::vector<Sense> senses = GetSenses(input, i);
      for (int senseIdx = 0; senseIdx < senses.size(); senseIdx++) {
        classifier.AddLabelIndependentFeature("snsa^" + GetWordForm(input, i) + "^" + SPrint(i - end + 1) + "^" + senses[senseIdx].m_word, senses[senseIdx].m_prob);
        classifier.AddLabelIndependentFeature("snsa^" + GetWordForm(input, i) + "^" + senses[senseIdx].m_word, senses[senseIdx].m_prob);
      }
    }
  }

  virtual void SetParameter(const std::string& key, const std::string& value) {
    if (key == "size") {
      m_size = Scan<size_t>(value);
    } else {
      VWFeatureSource::SetParameter(key, value);
    }
  }

private:
  static const int DEFAULT_WINDOW_SIZE = 3;

  struct Sense {
    std::string m_word;
    float m_prob;
  };

  std::vector<Sense> GetSenses(const InputType &input, size_t pos) const {
    std::string w = GetWord(input, pos);
    std::vector<std::string> senseTokens = Tokenize(w, "^");

    std::vector<Sense> out(senseTokens.size());
    for (size_t i = 0; i < senseTokens.size(); i++) {
      std::vector<std::string> senseColumns = Tokenize(senseTokens[i], ":");
      if (senseColumns.size() != 2) {
        UTIL_THROW2("VW :: bad format of sense distribution: " << senseTokens[i]);
      }
      out[i].m_word = senseColumns[0];
      out[i].m_prob = Scan<float>(senseColumns[1]);
    }

    return out;
  }

  // assuming that word surface form is always factor 0, output the word form
  inline std::string GetWordForm(const InputType &input, size_t pos) const {
    return input.GetWord(pos).GetString(0).as_string();
  }

  int m_size;
};

}
