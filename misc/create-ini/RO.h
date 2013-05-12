#pragma once

#include "Util.h"
#include "FF.h"

class RO : public FF
{
  static int s_index;
	std::string type, fileTypeSuffix;
	
  float GetWeight() const
  { return 0.3; }

  void Output(std::ostream &out) const
  {
    out << name
    		<< " num-features=" << numFeatures
    		<< " type=" << type
    		<< " input-factor=" << OutputFactors(inFactors)
    		<< " output-factor=" << OutputFactors(outFactors)
        << " path=" << path << "." << fileTypeSuffix << ".gz"
        << std::endl;
  }
public:

  RO(const std::string &line, const std::pair<Factors, Factors> *factors);

};

