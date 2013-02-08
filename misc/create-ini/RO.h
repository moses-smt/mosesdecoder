#pragma once

#include "Util.h"
#include "FF.h"

class RO : public FF
{
  static int s_index;

  float GetWeight() const
  { return 0.3; }

  void Output(std::ostream &out) const
  {
    out << name << index
        << " path=" << path
        << std::endl;
  }
public:

  RO(const std::string &line)
  :FF(line)
  {
    index = s_index++;
    name = "LexicalReordering";
    numFeatures = 6;
    path = toks[0];
  }

};

