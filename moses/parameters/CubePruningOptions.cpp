// -*- mode: c++; cc-style: gnu -*-
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

}
