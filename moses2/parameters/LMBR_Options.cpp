// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "LMBR_Options.h"
#include "../legacy/Parameter.h"

namespace Moses2
{

LMBR_Options::
LMBR_Options()
  : enabled(false)
  , use_lattice_hyp_set(false)
  , precision(0.8f)
  , ratio(0.6f)
  , map_weight(0.8f)
  , pruning_factor(30)
{ }

bool
LMBR_Options::
init(Parameter const& param)
{
  param.SetParameter(enabled, "lminimum-bayes-risk", false);

  param.SetParameter(ratio, "lmbr-r", 0.6f);
  param.SetParameter(precision, "lmbr-p", 0.8f);
  param.SetParameter(map_weight, "lmbr-map-weight", 0.0f);
  param.SetParameter(pruning_factor, "lmbr-pruning-factor", size_t(30));
  param.SetParameter(use_lattice_hyp_set, "lattice-hypo-set", false);

  PARAM_VEC const* params = param.GetParam("lmbr-thetas");
  if (params) theta = Scan<float>(*params);

  return true;
}




}
