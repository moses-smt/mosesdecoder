// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "CubePruningOptions.h"
#include "../TypeDef.h"
#include "../legacy/Parameter.h"

namespace Moses2
{

CubePruningOptions::
CubePruningOptions()
  : pop_limit(DEFAULT_CUBE_PRUNING_POP_LIMIT)
  , diversity(DEFAULT_CUBE_PRUNING_DIVERSITY)
  , lazy_scoring(false)
  , deterministic_search(false)
{}

bool
CubePruningOptions::
init(Parameter const& param)
{
  param.SetParameter(pop_limit, "cube-pruning-pop-limit",
                     DEFAULT_CUBE_PRUNING_POP_LIMIT);
  param.SetParameter(diversity, "cube-pruning-diversity",
                     DEFAULT_CUBE_PRUNING_DIVERSITY);
  param.SetParameter(lazy_scoring, "cube-pruning-lazy-scoring", false);
  //param.SetParameter(deterministic_search, "cube-pruning-deterministic-search", false);
  return true;
}

#ifdef HAVE_XMLRPC_C
bool
CubePruningOptions::
update(std::map<std::string,xmlrpc_c::value>const& params)
{
  typedef std::map<std::string, xmlrpc_c::value> params_t;

  params_t::const_iterator si = params.find("cube-pruning-pop-limit");
  if (si != params.end()) pop_limit = xmlrpc_c::value_int(si->second);

  si = params.find("cube-pruning-diversity");
  if (si != params.end()) diversity = xmlrpc_c::value_int(si->second);

  si = params.find("cube-pruning-lazy-scoring");
  if (si != params.end()) {
    std::string spec = xmlrpc_c::value_string(si->second);
    if (spec == "true" or spec == "on" or spec == "1")
      lazy_scoring = true;
    else if (spec == "false" or spec == "off" or spec == "0")
      lazy_scoring = false;
    else {
      char const* msg
      = "Error parsing specification for cube-pruning-lazy-scoring";
      xmlrpc_c::fault(msg, xmlrpc_c::fault::CODE_PARSE);
    }
  }

  si = params.find("cube-pruning-deterministic-search");
  if (si != params.end()) {
    std::string spec = xmlrpc_c::value_string(si->second);
    if (spec == "true" or spec == "on" or spec == "1")
      deterministic_search = true;
    else if (spec == "false" or spec == "off" or spec == "0")
      deterministic_search = false;
    else {
      char const* msg
      = "Error parsing specification for cube-pruning-deterministic-search";
      xmlrpc_c::fault(msg, xmlrpc_c::fault::CODE_PARSE);
    }
  }

  return true;
}
#endif


}
