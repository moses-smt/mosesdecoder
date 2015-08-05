// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
namespace Moses
{

  bool is_syntax(SearchAlgorithm algo);
  
  struct 
  SearchOptions 
  {
    SearchAlgorithm algo;
    
    // stack options
    size_t stack_size;      // maxHypoStackSize;
    size_t stack_diversity; // minHypoStackDiversity;

    // beam search
    float beam_width;

    // reordering options
    // bool  reorderingConstraint; //! use additional reordering constraints
    // bool  useEarlyDistortionCost;

    float early_discarding_threshold;
    float translationOptionThreshold;

    bool init(Parameter const& param);
    SearchOptions(Parameter const& param);
    SearchOptions() {}
  };

}
