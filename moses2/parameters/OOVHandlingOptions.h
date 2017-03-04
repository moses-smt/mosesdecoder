// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include <string>
#include "OptionsBaseClass.h"

namespace Moses2
{
struct
    OOVHandlingOptions : public OptionsBaseClass {
  bool drop;
  bool mark;
  std::string prefix;
  std::string suffix;

  bool word_deletion_enabled;
  bool always_create_direct_transopt;
  OOVHandlingOptions();

  bool init(Parameter const& param);
  bool update(std::map<std::string,xmlrpc_c::value>const& param);

};

}

