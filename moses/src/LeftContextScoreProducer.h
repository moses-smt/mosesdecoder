// $Id: LeftContextScoreProducer.h,v 1.1 2012/10/07 13:43:03 braunefe Exp $

#ifndef moses_LeftContextScoreProducer_h
#define moses_LeftContextScoreProducer_h

#include "FeatureFunction.h"
#include "TypeDef.h"
#include <map>
#include <string>
#include <vector>

namespace Moses
{

typedef std::map<std::pair<std::string, std::string>, float> TTable;

class LeftContextScoreProducer : public StatefulFeatureFunction
{
public:
  LeftContextScoreProducer(ScoreIndexManager &scoreIndexManager, float weight);

  // read the extended phrase table
  void LoadScores(const std::string &ttableFile);

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
  TTable modelScores;
  std::vector<FactorType> srcFactors, tgtFactors; // which factors to use; XXX hard-coded for now
};

}

#endif // moses_LeftContextScoreProducer_h
