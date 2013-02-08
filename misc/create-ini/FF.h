#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "Util.h"

class FF
{
  virtual float GetWeight() const = 0;

public:
  std::vector<std::string> toks;
  std::string name;
  std::string path;
  int index, numFeatures;
  
  FF(const std::string &line)
  {
    toks = Tokenize(line, ":");
  }

  virtual void Output(std::ostream &out) const = 0;
  virtual void OutputWeights(std::ostream &out) const
  {
    out << name << index << "= ";
    for (size_t i = 0; i < numFeatures; ++i) {
      out << GetWeight() << " ";
    }
    out << std::endl;
  }

};


