#pragma once

#include <cstddef>
#include <vector>
#include <iostream>

class Hypothesis {
 public:
    Hypothesis(size_t word, size_t prev, float cost)
      : prev_(prev),
        word_(word),
        cost_(cost) {
    }

    size_t GetWord() const {
      return word_;
    }

    size_t GetPrevStateIndex() const {
      return prev_;
    }

    float GetCost() const {
      return cost_;
    }

 private:
    const size_t prev_;
    const size_t word_;
    const float cost_;
};

