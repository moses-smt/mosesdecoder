
#include <string>

#include "VWFeatureBase.h"

namespace Moses
{
  std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_features;
  std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_sourceFeatures;
  std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_targetFeatures;
}

