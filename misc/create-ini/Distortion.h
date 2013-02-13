#pragma once

#include "FF.h"

class Distortion : public FF
{
  static int s_index;

  float GetWeight() const
  { return 0.3; }

  void Output(std::ostream &out) const
  {
    out << name
        << std::endl;
  }
public:
  Distortion(const std::string &line)
  :FF(line)
  {
    index = s_index++;
    name = "Distortion";
    numFeatures = 1; 
  }

};
