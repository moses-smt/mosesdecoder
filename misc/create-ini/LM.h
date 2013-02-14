#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "Util.h"
#include "FF.h"

class LM : public FF
{
  static int s_index;

  float GetWeight() const
  { return 0.5; }

  void Output(std::ostream &out) const
  {
    out << name
        << " order=" << order 
        << " factor=" << OutputFactors(outFactors)
        << " path=" << path
        << " " << otherArgs
        << std::endl;
  }

public:
  std::string otherArgs;
  int order;

  LM(const std::string &line)
  :FF(line)
  {
    index = s_index++;
    numFeatures = 1;

    outFactors.push_back(Scan<int>(toks[0]));
    order = Scan<int>(toks[1]);
    path = toks[2];
    int implNum = 0;
    if (toks.size() >= 4)
      implNum = Scan<int>(toks[3]);

    switch (implNum)
    {
      case 0: name = "SRILM"; break;
      case 1: name = "IRSTLM"; break;
      case 8: name = "KENLM"; otherArgs = "lazyken=0"; break;
      case 9: name = "KENLM"; otherArgs = "lazyken=1"; break;
    }
  }

};
