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

  // output
  typedef std::vector<std::pair<Moses::Word, Moses::WordsRange> > ApplicationContext;
  typedef std::set< std::pair<size_t, size_t>  > Alignments;

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

  template <class T>
  void ShiftOffsets(std::vector<T> &offsets, T shift) const
  {
    T currPos = shift;
    for (size_t i = 0; i < offsets.size(); ++i) {
      if (offsets[i] == 0) {
        offsets[i] = currPos;
        ++currPos;
      } else {
        currPos += offsets[i];
      }
    }
  }

public:
  // outputs
  virtual void OutputNBest(OutputCollector *collector) const = 0;
  virtual void OutputLatticeSamples(OutputCollector *collector) const = 0;
  virtual void OutputAlignment(OutputCollector *collector) const = 0;
  virtual void OutputDetailedTranslationReport(OutputCollector *collector) const = 0;
  virtual void OutputDetailedTreeFragmentsTranslationReport(OutputCollector *collector) const = 0;


};

}
