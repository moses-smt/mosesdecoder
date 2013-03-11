#pragma once

#include <vector>
#include <string>
#include "Util.h"
#include "FF.h"

class PT : public FF
{
  static int s_index;


  float GetWeight() const
  { return 0.2; }

  void Output(std::ostream &out) const;

public:
  PT(const std::string &line, int numFeatures, bool isHierarchical, const std::pair<Factors, Factors> *factors);

  void OutputWeights(std::ostream &out) const;
};
