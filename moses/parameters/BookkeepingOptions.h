// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "moses/Parameter.h"
namespace Moses
{

  struct BookkeepingOptions {
    bool need_alignment_info;
    bool init(Parameter const& param);
  };
  


}
