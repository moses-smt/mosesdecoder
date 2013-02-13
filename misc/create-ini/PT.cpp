#include "PT.h"

int PT::s_index = 0;

void PT::Output(std::ostream &out) const
{
  out << name
      << " implementation=" << 0
      << " num-features=" << numFeatures
      << " path=" << path;

  out << " input-factor=" << OutputFactors(inFactors);
  out << " output-factor=" << OutputFactors(outFactors);

  out << std::endl;
}

