// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include "moses/Parameter.h"
#include "OptionsBaseClass.h"

namespace Moses
{

  struct BookkeepingOptions : public OptionsBaseClass 
  {
    bool need_alignment_info;
    bool init(Parameter const& param);
    BookkeepingOptions();
  };
  


}
