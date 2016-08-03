#pragma once

#include <vector>
#include <iostream>

#include "decoder/hypothesis.h"

class HypothesisManager {
    using Hypotheses = std::vector<Hypothesis>;
 public:
    HypothesisManager(size_t beamSize, size_t EOSIndex)
      : beamSize_(beamSize),
        EOSIndex_(EOSIndex),
        baseIndex_(0) {
        hypotheses_.emplace_back(0, 0, 0);
    }

    void AddHypotheses(const Hypotheses& hypos) {
      size_t nextBaseIndex = hypotheses_.size();
      for (const auto& hypo : hypos) {
        if (hypo.GetWord() == EOSIndex_) {
          completedHypotheses_.emplace_back(hypo.GetWord(),
                                            hypo.GetPrevStateIndex() + baseIndex_,
                                            hypo.GetCost());
        } else {
          hypotheses_.emplace_back(hypo.GetWord(), hypo.GetPrevStateIndex() + baseIndex_,
                                   hypo.GetCost());
        }
      }
      baseIndex_ = nextBaseIndex;
    }

    std::vector<size_t> GetBestTranslation() {
      size_t bestHypoId = 0;
      for (size_t i = 0; i < completedHypotheses_.size(); ++i) {
        if (completedHypotheses_[bestHypoId].GetCost()
            < completedHypotheses_[i].GetCost()) {
          bestHypoId = i;
        }
      }


      // for (auto hypo : completedHypotheses_) {
        // std::vector<size_t> words;
        // words.push_back(hypo.GetWord());
        // size_t state = hypo.GetPrevStateIndex();
        // while (state > 0)  {
          // words.push_back(hypotheses_[state].GetWord());
          // state = hypotheses_[state].GetPrevStateIndex();
        // }
        // for (auto it = words.rbegin(); it != words.rend(); ++it) std::cerr << *it << " ";
        // std::cerr << hypo.GetCost() << std::endl;
      // }

      std::vector<size_t> bestSentence;
      bestSentence.push_back(completedHypotheses_[bestHypoId].GetWord());
      size_t state = completedHypotheses_[bestHypoId].GetPrevStateIndex();

      while (state > 0) {
            bestSentence.push_back(hypotheses_[state].GetWord());
        state = hypotheses_[state].GetPrevStateIndex();
      }

      return bestSentence;
    }

 private:
    Hypotheses hypotheses_;
    size_t beamSize_;
    Hypotheses completedHypotheses_;
    const size_t EOSIndex_;
    size_t baseIndex_;
};


