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

  void Output(std::ostream &out) const
  {
    out << name << index << " "
        << " path=" << path
        << std::endl;
  }
public:
  int numFeatures;
  std::vector<int> inFactor, outFactor;

  PT(const std::string &line)
  :FF(line)
  {
    index = s_index++;
    name = "PhraseModel";
    numFeatures = 5;    
    path = toks[0];

    inFactor.push_back(0);
    outFactor.push_back(0);
  }

};
