#pragma once

#include <iostream>
#include <string>
#include "ScoreComponentCollection.h"

namespace Moses
{
class ScoreComponentCollection;
class FeatureFunction;
class OutputCollector;

class BaseManager
{
protected:
  void OutputAllFeatureScores(const Moses::ScoreComponentCollection &features,
							  std::ostream &out) const;
  void OutputFeatureScores( std::ostream& out,
							const ScoreComponentCollection &features,
							const FeatureFunction *ff,
							std::string &lastName ) const;
  void OutputSurface(std::ostream &out,
		  	  	  	  const Phrase &phrase,
		  	  	  	  const std::vector<FactorType> &outputFactorOrder,
		  	  	  	  bool reportAllFactors) const;

public:
  // outputs
  virtual void OutputNBest(OutputCollector *collector) const = 0;

};

}
