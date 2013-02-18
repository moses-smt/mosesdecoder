#pragma once

#include <vector>
#include <string>
#include "Util.h"
#include "FF.h"

class PT : public FF
{
  static int s_index;

  int implementation;

  float GetWeight() const
  { return 0.2; }

  void Output(std::ostream &out) const;
  void Load(const std::string &line, int numFeatures);

public:
  PT(const std::string &line);
  PT(const std::string &line, int numFeatures);
};
