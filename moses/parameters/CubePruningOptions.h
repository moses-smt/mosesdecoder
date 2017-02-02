// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include "OptionsBaseClass.h"
namespace Moses
{

  struct 
  CubePruningOptions : public OptionsBaseClass
  {
    size_t  pop_limit;
    size_t  diversity;
    bool lazy_scoring;
    bool deterministic_search;

    bool init(Parameter const& param);
    CubePruningOptions(Parameter const& param);
    CubePruningOptions();

    bool 
    update(std::map<std::string,xmlrpc_c::value>const& params);
  };

}
