
#include <string>

#include "VWFeatureBase.h"

namespace Moses
{
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_features;
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_sourceFeatures;
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_targetContextFeatures;
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_targetFeatures;

const std::string VWFeatureBase::BOS_STRING = "<S>";
const std::string VWFeatureBase::EOS_STRING = "</S>";
}

