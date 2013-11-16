#include "StatelessFeatureFunction.h"

namespace Moses
{

std::vector<const StatelessFeatureFunction*> StatelessFeatureFunction::m_statelessFFs;

StatelessFeatureFunction::StatelessFeatureFunction(const std::string &line)
  :FeatureFunction(line)
{
  m_statelessFFs.push_back(this);
}

StatelessFeatureFunction::StatelessFeatureFunction(size_t numScoreComponents, const std::string &line)
  :FeatureFunction(numScoreComponents, line)
{
  m_statelessFFs.push_back(this);
}

}

