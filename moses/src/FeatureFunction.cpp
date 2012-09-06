#include "FeatureFunction.h"

#include "util/check.hh"

namespace Moses
{

FeatureFunction::~FeatureFunction() {}

bool StatelessFeatureFunction::IsStateless() const
{
  return true;
}

bool StatelessFeatureFunction::ComputeValueInTranslationOption() const
{
  return false;
}

bool StatefulFeatureFunction::IsStateless() const
{
  return false;
}

}

