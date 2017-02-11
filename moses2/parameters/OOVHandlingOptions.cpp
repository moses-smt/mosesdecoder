// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "OOVHandlingOptions.h"
#include <vector>
#include <iostream>
#include "../legacy/Parameter.h"

namespace Moses2
{

OOVHandlingOptions::
OOVHandlingOptions()
{
  drop = false;
  mark = false;
  prefix = "UNK";
  suffix = "";
  word_deletion_enabled = false;
  always_create_direct_transopt = false;
}

bool
OOVHandlingOptions::
init(Parameter const& param)
{
  param.SetParameter(drop,"drop-unknown",false);
  param.SetParameter(mark,"mark-unknown",false);
  param.SetParameter(word_deletion_enabled, "phrase-drop-allowed", false);
  param.SetParameter(always_create_direct_transopt, "always-create-direct-transopt", false);
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
