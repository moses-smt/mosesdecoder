#pragma once

#include "InputFeature.h"
#include "StatelessFeatureFunction.h"

namespace Moses
{


class InputFeature : public StatelessFeatureFunction
{


public:
  InputFeature(const std::string &line);

  // miscellaenous ad-hoc features
  static const InputFeature &GetInputFeature();

};


}

