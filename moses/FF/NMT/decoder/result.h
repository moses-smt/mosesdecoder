#pragma once

#include <cstddef>

struct Result {
  Result(const size_t state, const size_t word, const float score)
    : state(state),
      word(word),
      score(score) {
  }

  size_t state;
  size_t word;
  float score;
};
