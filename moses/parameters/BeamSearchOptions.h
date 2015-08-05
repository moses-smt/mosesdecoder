// -*- mode: c++; cc-style: gnu -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
namespace Moses
{

  struct 
  BeamSearchOptions 
  {
    bool init(Parameter const& param);
    BeamSearchOptions(Parameter const& param);
  };

}
