// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "AllOptions.h"

namespace Moses
{
  AllOptions::
  AllOptions(Parameter const& param)
  {
    init(param);
  }

  bool
  AllOptions::
  init(Parameter const& param)
  {
    if (!search.init(param))     return false;
    if (!cube.init(param))       return false;
    if (!nbest.init(param))      return false;
    if (!reordering.init(param)) return false;
    if (!context.init(param))    return false;
    if (!input.init(param))      return false;
    if (!mbr.init(param))        return false;
    if (!lmbr.init(param))       return false;

    param.SetParameter(mira, "mira", false);

    return sanity_check();
  }

  bool
  AllOptions::
  sanity_check()
  {
    using namespace std;
    if (lmbr.enabled)
      {
        if (mbr.enabled) 
          {
            cerr << "Error: Cannot use both n-best mbr and lattice mbr together" << endl;
            return false;
          }
        mbr.enabled = true;
      }
    if (search.consensus)
      {
        if (mbr.enabled) 
          {
            cerr << "Error: Cannot use consensus decoding together with mbr" << endl;
            return false;
          }
        mbr.enabled = true;
      }

    return true;
  }

#ifdef HAVE_XMLRPC_C
  bool 
  AllOptions::
  update(std::map<std::string,xmlrpc_c::value>const& param)
  {
    if (!search.update(param))     return false;
    if (!cube.update(param))       return false;
    if (!nbest.update(param))      return false;
    if (!reordering.update(param)) return false;
    if (!context.update(param))    return false;
    if (!input.update(param))      return false;
    if (!mbr.update(param))        return false;
    if (!lmbr.update(param))       return false;
    return true;
  }
#endif


}
