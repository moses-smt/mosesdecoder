// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "MBR_Options.h"

namespace Moses {

  MBR_Options::
  MBR_Options() 
    : enabled(false)
    , size(200)
    , scale(1.0f)
  {}


  bool
  MBR_Options::
  init(Parameter const& param)
  {
    param.SetParameter(enabled, "minimum-bayes-risk", false);
    param.SetParameter<size_t>(size, "mbr-size", 200);
    param.SetParameter(scale, "mbr-scale", 1.0f);
    return true;
  }

}
