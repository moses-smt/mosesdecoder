#include "PT.h"

int PT::s_index = 0;

PT::PT(const std::string &line, int numFeatures, bool isHierarchical)
:FF(line)
{
  index = s_index++;
  name = "PhraseModel";
  this->numFeatures = numFeatures;    
  path = toks[0];

  inFactors.push_back(0);
  outFactors.push_back(0);

  if (toks.size() > 1)
    implementation = Scan<int>(toks[1]);
  else if (isHierarchical)
    implementation = 6;
  else
    implementation = 0;
}

void PT::Output(std::ostream &out) const
{
  out << name
      << " implementation=" << implementation
      << " num-features=" << numFeatures
      << " path=" << path;

  out << " input-factor=" << OutputFactors(inFactors);
  out << " output-factor=" << OutputFactors(outFactors);

  out << std::endl;
}

