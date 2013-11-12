
#pragma once

#include "FeatureFunction.h"

class StatelessFeatureFunction : public FeatureFunction
{
public:
  static const std::vector<StatelessFeatureFunction*>& GetColl() {
    return s_staticColl;
  }

  StatelessFeatureFunction(const std::string line);
  virtual ~StatelessFeatureFunction();

protected:
  static std::vector<StatelessFeatureFunction*> s_staticColl;

};

