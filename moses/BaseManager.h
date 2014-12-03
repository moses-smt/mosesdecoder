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
  typedef std::vector<std::pair<Moses::Word, Moses::WordsRange> > ApplicationContext;

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
  void WriteApplicationContext(std::ostream &out,
                                          const ApplicationContext &context) const;

public:
  // outputs
  virtual void OutputNBest(OutputCollector *collector) const = 0;
  virtual void OutputLatticeSamples(OutputCollector *collector) const = 0;
  virtual void OutputAlignment(OutputCollector *collector) const = 0;
  virtual void OutputDetailedTranslationReport(OutputCollector *collector) const = 0;


};

}
