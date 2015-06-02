// -*- mode: c++; cc-style: gnu -*-
#include "moses/Parameter.h"
#include "NBestOptions.h"

namespace Moses {

bool
NBestOptions::
init(Parameter const& P)
{
  const PARAM_VEC *params;
  params = P.GetParam("n-best-list");
  if (params)
    {
      if (params->size() >= 2)
	{
	  output_file_path = params->at(0);
	  nbest_size = Scan<size_t>( params->at(1) );
	  only_distinct = (params->size()>2 && params->at(2)=="distinct");
	}
      else
	{
	  std::cerr << "wrong format for switch -n-best-list file size [disinct]";
	  return false;
	}
    }
  else nbest_size = 0;

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
} // namespace Moses
