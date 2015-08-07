// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "OptionsBaseClass.h"

namespace Moses
{
#ifdef HAVE_XMLRPC_C
  bool   
  OptionsBaseClass::
  update(std::map<std::string,xmlrpc_c::value>const& params)
  {
    return true;
  }
#endif

}
