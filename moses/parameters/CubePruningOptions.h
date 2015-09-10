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

    bool init(Parameter const& param);
    CubePruningOptions(Parameter const& param);
    CubePruningOptions() {};

#ifdef HAVE_XMLRPC_C
    bool 
    update(std::map<std::string,xmlrpc_c::value>const& params);
#endif
  };

}
