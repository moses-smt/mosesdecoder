// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include "../legacy/xmlrpc-c.h"
#include <string>
#include <map>
namespace Moses2
{
class Parameter;

struct OptionsBaseClass {
#ifdef HAVE_XMLRPC_C
  virtual bool
  update(std::map<std::string,xmlrpc_c::value>const& params);
#endif
  bool
  check(std::map<std::string, xmlrpc_c::value> const& param,
        std::string const key, bool dfltval);
};
}
