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
  void Output(std::ostream &out, const std::vector<int> &factors) const;
public:
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
