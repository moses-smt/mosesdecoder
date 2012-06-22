// $Id$

#ifndef moses_PSDScoreProducer_h
#define moses_PSDScoreProducer_h

#include "FeatureFunction.h"
#include "vw.h"
#include "TypeDef.h"
#include <map>
#include <string>
#include <vector>

struct vw;

namespace Moses
{

typedef std::map<std::pair<std::string, std::string>, float> TTable;

class PSDScoreProducer : public StatefulFeatureFunction
{
public:
  PSDScoreProducer(ScoreIndexManager &scoreIndexManager, float weight);

  // read the extended phrase table
  void Initialize(const std::string &modelFile);

  // mandatory methods for features
  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  // maintain hypothesis state (currently not used)
  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  // score translation hypothesis
  virtual FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  // not implemented yet
  virtual FFState* EvaluateChart(
    const ChartHypothesis&,
    int /* featureID */,
    ScoreComponentCollection*) const {
		CHECK(0); // feature function not valid in chart decoder
		return NULL;
	}
private:
  feature feature_from_string(const string &feature_str, unsigned long seed, float value);  

  ::vw vw;
  std::vector<FactorType> srcFactors, tgtFactors; // which factors to use; XXX hard-coded for now
};

}

#endif // moses_PSDScoreProducer_h
