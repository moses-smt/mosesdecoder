
#pragma once

#include <vector>
#include <string>
#include "TypeDef.h"

class FeatureFunction;

class Weights
{
public:
  Weights();
  virtual ~Weights();
  void CreateFromString(const std::string &line);

  const std::vector<SCORE> &GetWeights() const {
    return m_weights;
  }

  void SetWeights(const FeatureFunction &ff, const std::vector<SCORE> &weights);
  void SetNumScores(size_t num) {
    m_weights.resize(num, 0);
  }

protected:
  std::vector<SCORE> m_weights;
};

