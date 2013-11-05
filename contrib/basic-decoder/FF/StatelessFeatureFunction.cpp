
#include "StatelessFeatureFunction.h"

std::vector<StatelessFeatureFunction*> StatelessFeatureFunction::s_staticColl;

StatelessFeatureFunction::StatelessFeatureFunction(const std::string line)
  :FeatureFunction(line)
{
  s_staticColl.push_back(this);
}

StatelessFeatureFunction::~StatelessFeatureFunction()
{
  // TODO Auto-generated destructor stub
}

