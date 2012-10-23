// $Id$

#ifndef moses_ContextSimilarityScoreProducer_h
#define moses_ContextSimilarityScoreProducer_h

#include "FeatureFunction.h"
#include <string>
#include <set>

namespace Moses
{

class ContextSimilarityScoreProducer : public StatefulFeatureFunction
{
public:
	ContextSimilarityScoreProducer() : StatefulFeatureFunction("ContextSimilarity", 1) { }

	std::string GetScoreProducerWeightShortName(unsigned) const { return "csim"; }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* Evaluate(const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart(const ChartHypothesis&, int, ScoreComponentCollection*) const {
		CHECK(0);
		return NULL;
	}

private:
  float CosineSimilarity(const std::set<std::string> &a, const std::set<std::string> &b) const;
};

}
#endif
