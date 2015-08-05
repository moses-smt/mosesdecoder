// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "AllOptions.h"

namespace Moses
{
  AllOptions::
  AllOptions(Parameter const& param)
  {
    init(param);
  }

  bool
  AllOptions::
  init(Parameter const& param)
  {
    if (!search.init(param))     return false;
    if (!cube.init(param))       return false;
    if (!nbest.init(param))      return false;
    if (!reordering.init(param)) return false;
    if (!context.init(param))    return false;
    if (!input.init(param))      return false;
    return sanity_check();
  }

  bool
  AllOptions::
  sanity_check()
  {
    return true;
  }
}
