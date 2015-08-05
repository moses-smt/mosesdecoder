// -*- mode: c++; cc-style: gnu -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include "SearchOptions.h"
#include "CubePruningOptions.h"
#include "NBestOptions.h"
#include "ReorderingOptions.h"
#include "ContextParameters.h"
#include "InputOptions.h"

namespace Moses
{
  struct 
  AllOptions 
  {
    SearchOptions         search;
    CubePruningOptions      cube;
    NBestOptions           nbest;
    ReorderingOptions reordering;
    ContextParameters    context;
    InputOptions           input;
    // StackOptions      stack;
    // BeamSearchOptions  beam;
    bool init(Parameter const& param);
    bool sanity_check();
    AllOptions() {}
    AllOptions(Parameter const& param);
  };

}
