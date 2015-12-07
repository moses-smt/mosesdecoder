// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include <string>
#include "OptionsBaseClass.h"

namespace Moses
{
  struct 
  SyntaxOptions : public OptionsBaseClass
  {
    S2TParsingAlgorithm s2t_parsing_algo;

    SyntaxOptions();

    bool init(Parameter const& param);
    bool update(std::map<std::string,xmlrpc_c::value>const& param);

  };

}

