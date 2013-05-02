#pragma once

#include "FF.h"

class UnknownWP : public FF
{
  static int s_index;

  virtual float GetWeight() const
  { return 1.0; }

  void Output(std::ostream &out) const
  {
    out << name
        << std::endl;
  }

public:
  UnknownWP(const std::string &line)
  :FF(line)
  {
    index = s_index++;
    name = "UnknownWordPenalty";
    numFeatures = 1; 
  }
};
