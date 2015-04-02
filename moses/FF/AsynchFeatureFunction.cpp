#include "AsynchFeatureFunction.h"

namespace Moses
{

std::vector<const AsynchFeatureFunction*> AsynchFeatureFunction::m_AsynchFFs;

AsynchFeatureFunction::AsynchFeatureFunction(const std::string &line)
  :FeatureFunction(line)
{
  m_AsynchFFs.push_back(this);
}

AsynchFeatureFunction::AsynchFeatureFunction(size_t numScoreComponents, const std::string &line)
  :FeatureFunction(numScoreComponents, line)
{
  m_AsynchFFs.push_back(this);
}

}

