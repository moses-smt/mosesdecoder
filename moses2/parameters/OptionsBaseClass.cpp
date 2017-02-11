// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "OptionsBaseClass.h"
#include "../legacy/Parameter.h"

namespace Moses2
{

#ifdef HAVE_XMLRPC_C
bool
OptionsBaseClass::
update(std::map<std::string,xmlrpc_c::value>const& params)
{
  return true;
}
#endif

#ifdef HAVE_XMLRPC_C
bool
OptionsBaseClass::
check(std::map<std::string, xmlrpc_c::value> const& param,
      std::string const key, bool dfltval)
{
  std::map<std::string, xmlrpc_c::value>::const_iterator m;
  m = param.find(key);
  if (m == param.end()) return dfltval;
  return Scan<bool>(xmlrpc_c::value_string(m->second));
}
#endif
}
