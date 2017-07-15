// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#include "ReorderingOptions.h"
#include "../legacy/Parameter.h"

namespace Moses2
{

ReorderingOptions::
ReorderingOptions()
  : max_distortion(-1)
  , monotone_at_punct(false)
  , use_early_distortion_cost(false)
{}


ReorderingOptions::
ReorderingOptions(Parameter const& param)
{
  init(param);
}

bool
ReorderingOptions::
init(Parameter const& param)
{
  param.SetParameter(max_distortion, "distortion-limit", -1);
  param.SetParameter(monotone_at_punct, "monotone-at-punctuation", false);
  param.SetParameter(use_early_distortion_cost, "early-distortion-cost", false);
  return true;
}
}
