#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "Util.h"

class FF
{
  virtual float GetWeight() const = 0;

protected:
  std::string OutputFactors(const std::vector<int> &factors) const;

public:
  std::vector<std::string> toks;
  std::vector<int> inFactors, outFactors;
  std::string name;
  std::string path;
  int index, numFeatures;
  
  FF(const std::string &line)
  {
    toks = Tokenize(line, ":");
  }

  virtual void Output(std::ostream &out) const = 0;
  virtual void OutputWeights(std::ostream &out) const;

};


