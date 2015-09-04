// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "CubePruningOptions.h"

namespace Moses 
{

  bool
  CubePruningOptions::
  init(Parameter const& param)
  {
    param.SetParameter(pop_limit, "cube-pruning-pop-limit",
		       DEFAULT_CUBE_PRUNING_POP_LIMIT);
    param.SetParameter(diversity, "cube-pruning-diversity",
		       DEFAULT_CUBE_PRUNING_DIVERSITY);
    param.SetParameter(lazy_scoring, "cube-pruning-lazy-scoring", false);
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
      if (si != params.end()) 
	{
	  std::string spec = xmlrpc_c::value_string(si->second);
	  if (spec == "true" or spec == "on" or spec == "1")
	    lazy_scoring = true;
	  else if (spec == "false" or spec == "off" or spec == "0")
	    lazy_scoring = false;
	  else 
	    {
	      char const* msg 
		= "Error parsing specification for cube-pruning-lazy-scoring";
	      xmlrpc_c::fault(msg, xmlrpc_c::fault::CODE_PARSE);
	    }
	}
      return true;
    }
#endif


}
