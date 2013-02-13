#pragma once

#include "FF.h"

class WP : public FF
{
  static int s_index;

  virtual float GetWeight() const
  { return 0.5; }

  void Output(std::ostream &out) const
  {
    out << name
        << std::endl;
  }

public:
  WP(const std::string &line)
  :FF(line)
  {
    index = s_index++;
    name = "WordPenalty";
    numFeatures = 1; 
  }
};
