#include <stdexcept>

#include "InputFeature.h"

using namespace std;

namespace Moses
{
InputFeature::InputFeature(const std::string &line)
  :StatelessFeatureFunction("InputFeature", line)
{

}

const InputFeature &InputFeature::GetInputFeature()
{
  static const InputFeature *staticObj = NULL;

  if (staticObj) {
    return *staticObj;
  }

  // 1st time looking up the feature
  const std::vector<const StatelessFeatureFunction*> &statefulFFs = StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for (size_t i = 0; i < statefulFFs.size(); ++i) {
    const StatelessFeatureFunction *ff = statefulFFs[i];
    const InputFeature *lm = dynamic_cast<const InputFeature*>(ff);

    if (lm) {
      staticObj = lm;
      return *staticObj;
    }
  }

  throw std::logic_error("No input feature.");

}

}

