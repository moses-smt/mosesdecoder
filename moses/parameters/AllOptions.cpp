// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "AllOptions.h"

namespace Moses
{
  AllOptions::
  AllOptions()
    : mira(false)
    , use_legacy_pt(false)
  { }

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
    if (!output.init(param))     return false;
    if (!unk.init(param))        return false;
    if (!syntax.init(param))     return false;

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
            cerr << "Error: Cannot use consensus decoding together with mbr" 
                 << endl;
            return false;
          }
        mbr.enabled = true;
      }

    // RecoverPath should only be used with confusion net or word lattice input
    if (output.RecoverPath && input.input_type == SentenceInput)
      {
        TRACE_ERR("--recover-input-path should only be used with "
                  <<"confusion net or word lattice input!\n");
        output.RecoverPath = false;
      }

    // set m_nbest_options.enabled = true if necessary:
    nbest.enabled = (nbest.enabled || mira || search.consensus 
                     || nbest.nbest_size > 0
                     || mbr.enabled || lmbr.enabled
                     || !output.SearchGraph.empty()
                     || !output.SearchGraphExtended.empty()
                     || !output.SearchGraphSLF.empty()
                     || !output.SearchGraphHG.empty()
                     || !output.SearchGraphPB.empty()
                     || output.lattice_sample_size != 0);
    
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
    if (!output.update(param))     return false;
    if (!unk.update(param))        return false;
    if (!syntax.update(param))     return false;
    return sanity_check();
  }
#else
  bool 
  AllOptions::
  update(std::map<std::string,xmlrpc_c::value>const& param)
  {}
#endif

  bool
  AllOptions::
  NBestDistinct() const
  {
    return (nbest.only_distinct
            || mbr.enabled || lmbr.enabled
            || output.lattice_sample_size
            || !output.SearchGraph.empty()
            || !output.SearchGraphExtended.empty()
            || !output.SearchGraphSLF.empty()
            || !output.SearchGraphHG.empty());
  }

  
}
