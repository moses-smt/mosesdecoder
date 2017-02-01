// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "OptionsBaseClass.h"

namespace Moses2
{

struct
    LookupOptions : public OptionsBaseClass {
  bool init(Parameter const& param);
  LookupOptions() {}
};

}

