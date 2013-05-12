#include "PT.h"

using namespace std;

int PT::s_index = 0;

PT::PT(const std::string &line, int numFeatures, bool isHierarchical, const pair<Factors, Factors> *factors)
:FF(line)
{
  index = s_index++;
  this->numFeatures = numFeatures;    
  path = toks[0];

  if (factors) {
    inFactors = factors->first;
    outFactors = factors->second;
  }

  if (inFactors.size() == 0) {
    inFactors.push_back(0);
  }
  if (outFactors.size() == 0) {
    outFactors.push_back(0);
  }

  int implementation;
  if (toks.size() > 1)
    implementation = Scan<int>(toks[1]);
  else if (isHierarchical)
    implementation = 6;
  else
    implementation = 0;

  switch (implementation)
  {
  case 0: name = "PhraseDictionaryMemory"; break;
  case 1: name = "PhraseDictionaryTreeAdaptor"; break;
  case 2: name = "PhraseDictionaryOnDisk"; break;
  case 6: name = "PhraseDictionaryMemory"; break;
  default:name = "UnknownPtImplementation"; break;
  }
}

void PT::Output(std::ostream &out) const
{
  out << name
      << " name=TranslationModel" << index
      << " num-features=" << numFeatures
      << " path=" << path;

  out << " input-factor=" << OutputFactors(inFactors);
  out << " output-factor=" << OutputFactors(outFactors);

  out << std::endl;
}

void PT::OutputWeights(std::ostream &out) const
{
  out << "TranslationModel" << index << "= ";
  for (size_t i = 0; i < numFeatures; ++i) {
    out << GetWeight() << " ";
  }
  out << std::endl;
}

