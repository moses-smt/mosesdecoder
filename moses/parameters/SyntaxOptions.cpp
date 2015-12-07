// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "SyntaxOptions.h"
#include <vector>
#include <iostream>
#include "moses/StaticData.h"
#include "moses/TypeDef.h"

namespace Moses {

  SyntaxOptions::
  SyntaxOptions()
  { 
  }

  bool
  SyntaxOptions::
  init(Parameter const& param)
  {
    param.SetParameter(s2t_parsing_algo, "s2t-parsing-algorithm", RecursiveCYKPlus);
    return true;
  }


#ifdef HAVE_XMLRPC_C
  bool 
  SyntaxOptions::
  update(std::map<std::string,xmlrpc_c::value>const& param)
  {
    typedef std::map<std::string, xmlrpc_c::value> params_t;
    // params_t::const_iterator si = param.find("xml-input");
    // if (si != param.end())
    //   xml_policy = Scan<XmlInputType>(xmlrpc_c::value_string(si->second));
    return true;
  }
#endif

}
