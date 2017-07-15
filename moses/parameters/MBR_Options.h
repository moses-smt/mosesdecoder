// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
#include "OptionsBaseClass.h"
namespace Moses
{

  // Options for mimum bayes risk decoding
  struct 
  MBR_Options : public OptionsBaseClass
  {
    bool enabled;
    size_t size; //! number of translation candidates considered
    float scale; /*! scaling factor for computing marginal probability 
                  *  of candidate translation */
    bool init(Parameter const& param);
    MBR_Options();
  };

}

