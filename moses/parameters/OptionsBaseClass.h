// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#ifdef HAVE_XMLRPC_C
#include <xmlrpc-c/base.hpp>
#endif
#include <string>
#include <map>
namespace Moses
{
  struct OptionsBaseClass 
  {
#ifdef HAVE_XMLRPC_C
    virtual bool   
    update(std::map<std::string,xmlrpc_c::value>const& params);
#endif
  };
}
