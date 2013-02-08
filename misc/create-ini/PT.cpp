#include "PT.h"

int PT::s_index = 0;

void PT::Output(std::ostream &out) const
{
  out << name << index << " "
      << "implementation=" << 0 << " "
      << "num-features=" << numFeatures << " "
      << "path=" << path << " ";


  out << "input-factor=";
  Output(out, inFactor);
  out << " ";

  out << "output-factor=";
  Output(out, outFactor);
  out << " ";

  out << std::endl;
}

void PT::Output(std::ostream &out, const std::vector<int> &factors) const
{
  out << factors[0];
  for (size_t i = 1; i < factors.size(); ++i) {
    out << "," << factors[1];
  }
}

