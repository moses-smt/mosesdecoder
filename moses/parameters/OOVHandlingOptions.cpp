// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "OOVHandlingOptions.h"
#include <vector>
#include <iostream>
#include "moses/StaticData.h"
#include "moses/TypeDef.h"

namespace Moses {

  OOVHandlingOptions::
  OOVHandlingOptions()
  { 
    drop = false;
    mark = false;
    prefix = "UNK";
    suffix = "";
  }

  bool
  OOVHandlingOptions::
  init(Parameter const& param)
  {
    param.SetParameter(drop,"drop-unknown",false);
    param.SetParameter(mark,"mark-unknown",false);
    param.SetParameter<std::string>(prefix,"unknown-word-prefix","UNK");
    param.SetParameter<std::string>(suffix,"unknown-word-suffix","");
    return true;
  }


#ifdef HAVE_XMLRPC_C
  bool 
  OOVHandlingOptions::
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
