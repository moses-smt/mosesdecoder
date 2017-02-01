// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "NBestOptions.h"
#include "../legacy/Parameter.h"

namespace Moses2
{

NBestOptions::
NBestOptions()
  : nbest_size(0)
  , factor(20)
  , enabled(false)
  , print_trees(false)
  , only_distinct(false)
  , include_alignment_info(false)
  , include_feature_labels(true)
  , include_segmentation(false)
  , include_passthrough(false)
  , include_all_factors(false)
{}


bool
NBestOptions::
init(Parameter const& P)
{
  const PARAM_VEC *params;
  params = P.GetParam("n-best-list");
  if (params) {
    if (params->size() >= 2) {
      output_file_path = params->at(0);
      nbest_size = Scan<size_t>( params->at(1) );
      only_distinct = (params->size()>2 && params->at(2)=="distinct");
    } else {
      std::cerr << "wrong format for switch -n-best-list file size [distinct]";
      return false;
    }
  } else nbest_size = 0;

  P.SetParameter<size_t>(factor, "n-best-factor", 20);
  P.SetParameter(include_alignment_info, "print-alignment-info-in-n-best", false );
  P.SetParameter(include_feature_labels, "labeled-n-best-list", true );
  P.SetParameter(include_segmentation, "include-segmentation-in-n-best", false );
  P.SetParameter(include_passthrough, "print-passthrough-in-n-best", false );
  P.SetParameter(include_all_factors, "report-all-factors-in-n-best", false );
  P.SetParameter(print_trees, "n-best-trees", false );

  enabled = output_file_path.size();
  return true;
}

#ifdef HAVE_XMLRPC_C
bool
NBestOptions::
update(std::map<std::string,xmlrpc_c::value>const& param)
{
  typedef std::map<std::string, xmlrpc_c::value> params_t;
  params_t::const_iterator si = param.find("nbest");
  if (si != param.end())
    nbest_size = xmlrpc_c::value_int(si->second);
  only_distinct = check(param, "nbest-distinct", only_distinct);
  enabled = (nbest_size > 0);
  return true;
}
#endif


} // namespace Moses
