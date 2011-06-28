#ifndef moses_RandomFeature_h
#define moses_RandomFeature_h

#include <map>

#include "FeatureFunction.h"



namespace Moses {

/**
  * Random-valued feature, used for testing optimisation algorithms.
  **/
class RandomFeature : public StatelessFeatureFunction {
public:
  RandomFeature(size_t numValue, size_t scaling);

  virtual void Evaluate(
    const TargetPhrase& cur_hypo,
    ScoreComponentCollection* accumulator) const;

  //Score producer methods
	virtual size_t GetNumScoreComponents() const;
//	virtual std::string GetScoreProducerDescription() const;
	virtual std::string GetScoreProducerWeightShortName() const;

private:
  size_t m_numValues;
  size_t m_scaling;

};


};

#endif
