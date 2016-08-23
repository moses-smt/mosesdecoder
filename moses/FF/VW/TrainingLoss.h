#pragma once

#include <set>
#include <cmath>
#include <string>

#include "moses/Util.h"
#include "moses/StaticData.h"
#include "moses/Phrase.h"

namespace Moses
{

/**
 * Calculation of training loss for VW.
 */
class TrainingLoss
{
public:
  virtual float operator()(const TargetPhrase &candidate, const TargetPhrase &correct, bool isCorrect) const = 0;
};

/**
 * Basic 1/0 training loss.
 */
class TrainingLossBasic : public TrainingLoss
{
public:
  virtual float operator()(const TargetPhrase &candidate, const TargetPhrase &correct, bool isCorrect) const {
    return isCorrect ? 0.0 : 1.0;
  }
};

/**
 * BLEU2+1 training loss.
 */
class TrainingLossBLEU : public TrainingLoss
{
public:
  virtual float operator()(const TargetPhrase &candidate, const TargetPhrase &correct, bool isCorrect) const {
    std::multiset<std::string> refNgrams;
    float precision = 1.0;

    for (size_t size = 1; size <= BLEU_N; size++) {
      for (int pos = 0; pos <= (int)correct.GetSize() - (int)size; pos++) {
        refNgrams.insert(MakeNGram(correct, pos, pos + size));
      }

      int confirmed = 1; // we're BLEU+1
      int total = 1;
      for (int pos = 0; pos <= (int)candidate.GetSize() - (int)size; pos++) {
        total++;
        std::string ngram = MakeNGram(candidate, pos, pos + size);
        std::multiset<std::string>::iterator it;
        if ((it = refNgrams.find(ngram)) != refNgrams.end()) {
          confirmed++;
          refNgrams.erase(it);
        }
      }
      precision *= (float)confirmed / total;
    }

    int c = candidate.GetSize();
    int r = correct.GetSize();

    float brevityPenalty = c < r ? exp((float)(1.0 - r) / c) : 1.0;

    return 1.0 - brevityPenalty * pow(precision, (float)1.0 / BLEU_N);
  }

private:
  std::string MakeNGram(const TargetPhrase &phrase, size_t start, size_t end) const {
    std::vector<std::string> words;
    while (start != end) {
      words.push_back(phrase.GetWord(start).GetString(StaticData::Instance().options()->output.factor_order, false));
      start++;
    }
    return Join(" ", words);
  }

  static const size_t BLEU_N = 2;
};

}

