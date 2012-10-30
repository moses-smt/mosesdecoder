#ifndef moses_SparsePhraseFeature_h
#define moses_SparsePhraseFeature_h

#include <stdexcept>

#include "FactorCollection.h"
#include "FeatureFunction.h"

namespace Moses
{

/**
  * Collection of sparse features attached to each phrase pair.
  **/
class SparsePhraseDictionaryFeature : public StatelessFeatureFunction {

public:
  SparsePhraseDictionaryFeature():
    StatelessFeatureFunction("stm", ScoreProducer::unlimited) {}

  void Evaluate(const PhraseBasedFeatureContext& context,
              ScoreComponentCollection* accumulator) const;
  
  void EvaluateChart(
    const ChartBasedFeatureContext& context,
    ScoreComponentCollection*) const {
    throw std::logic_error("SparsePhraseDictionaryFeature not valid in chart decoder");
	}

  // basic properties
	std::string GetScoreProducerWeightShortName(unsigned) const { return "stm"; }
	size_t GetNumInputScores() const { return 0; }

};


}


#endif
