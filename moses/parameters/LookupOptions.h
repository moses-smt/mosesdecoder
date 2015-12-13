// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include "OptionsBaseClass.h"
namespace Moses
{

  struct 
  LookupOptions : public OptionsBaseClass
  {
    bool init(Parameter const& param);
    ReorderingOptions() {}
  };

}

