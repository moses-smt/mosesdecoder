// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include <vector>
#include "moses/Parameter.h"
#include "OptionsBaseClass.h"
namespace Moses
{

  // Options for mimum bayes risk decoding
  struct 
  LMBR_Options : public OptionsBaseClass
  {
    bool enabled;
    bool use_lattice_hyp_set; //! to use nbest as hypothesis set during lattice MBR
    float precision; //! unigram precision theta - see Tromble et al 08 for more details
    float ratio;     //! decaying factor for ngram thetas - see Tromble et al 08
    float map_weight; //! Weight given to the map solution. See Kumar et al 09 
    size_t pruning_factor; //! average number of nodes per word wanted in pruned lattice
    std::vector<float> theta; //! theta(s) for lattice mbr calculation
    bool init(Parameter const& param);
    LMBR_Options();
  };

}

