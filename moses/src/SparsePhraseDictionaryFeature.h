#ifndef moses_SparsePhraseFeature_h
#define moses_SparsePhraseFeature_h

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
    StatelessFeatureFunction("stm") {}

  void Evaluate(const TargetPhrase& cur_phrase,
                ScoreComponentCollection* accumulator) const;
  
  // basic properties
	size_t GetNumScoreComponents() const { return ScoreProducer::unlimited; }
	std::string GetScoreProducerWeightShortName(unsigned) const { return "stm"; }
	size_t GetNumInputScores() const { return 0; }

};


}


#endif
