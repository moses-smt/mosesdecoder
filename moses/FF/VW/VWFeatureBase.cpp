
#include <string>

#include "VWFeatureBase.h"
#include "VWFeatureContext.h"

namespace Moses
{
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_features;
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_sourceFeatures;
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_targetContextFeatures;
std::map<std::string, std::vector<VWFeatureBase*> > VWFeatureBase::s_targetFeatures;

std::map<std::string, size_t> VWFeatureBase::s_targetContextLength;


void VWFeatureBase::UpdateContextSize(const std::string &usedBy)
{
  // using the standard map behavior here: if the entry does not
  // exist, it will be added and initialized to zero
  size_t currentSize = s_targetContextLength[usedBy];
  size_t newSize = static_cast<VWFeatureContext *const>(this)->GetContextSize();
  s_targetContextLength[usedBy] = std::max(currentSize, newSize);
}

}

