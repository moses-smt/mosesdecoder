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
    
    // stack decoding
    size_t stack_size;      // maxHypoStackSize;
    size_t stack_diversity; // minHypoStackDiversity;

    size_t max_phrase_length;
    size_t max_trans_opt_per_cov; 
    size_t max_partial_trans_opt;
    // beam search
    float beam_width;

    int timeout;

    // reordering options
    // bool  reorderingConstraint; //! use additional reordering constraints
    // bool  useEarlyDistortionCost;

    float early_discarding_threshold;
    float trans_opt_threshold;

    bool init(Parameter const& param);
    SearchOptions(Parameter const& param);
    SearchOptions() {}

    bool UseEarlyDiscarding() const {
      return early_discarding_threshold != -std::numeric_limits<float>::infinity();
    }

  };

}
