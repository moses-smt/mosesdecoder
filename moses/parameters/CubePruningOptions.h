// -*- mode: c++; cc-style: gnu -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
namespace Moses
{

  struct 
  CubePruningOptions 
  {
    size_t  pop_limit;
    size_t  diversity;
    bool lazy_scoring;

    bool init(Parameter const& param);
    CubePruningOptions(Parameter const& param);
    CubePruningOptions() {};
  };

}
