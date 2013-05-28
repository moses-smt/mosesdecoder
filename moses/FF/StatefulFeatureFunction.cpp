#include "StatefulFeatureFunction.h"

namespace Moses
{

StatefulFeatureFunction::StatefulFeatureFunction(const std::string& description, const std::string &line)
: FeatureFunction(description, line)
{
  m_statefulFFs.push_back(this);
}

StatefulFeatureFunction::StatefulFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line)
: FeatureFunction(description,numScoreComponents, line)
{
  m_statefulFFs.push_back(this);
}

}

