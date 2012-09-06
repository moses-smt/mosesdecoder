#ifndef moses_SpanLengthFeature_h
#define moses_SpanLengthFeature_h

#include "FeatureFunction.h"
// MARIA NO HABLAS GIT
namespace Moses {
  
class InputType;
class ScoreIndexManager;
class ScoreComponentCollection;

class SpanLengthFeature : public StatefulFeatureFunction {
public:
  SpanLengthFeature(ScoreIndexManager &scoreIndexManager, const std::vector<float> &weights);
  virtual size_t GetNumScoreComponents() const;
  virtual std::string GetScoreProducerDescription(unsigned) const;
  virtual std::string GetScoreProducerWeightShortName(unsigned) const;
  virtual size_t GetNumInputScores() const;
  
  virtual const FFState* EmptyHypothesisState(const InputType &input) const;
  
  virtual FFState* Evaluate(
    const Hypothesis& currentHypothesis,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  
  virtual FFState* EvaluateChart(
    const ChartHypothesis& chartHypothesis,
    int featureId,
    ScoreComponentCollection* accumulator) const;
  
  bool WithTargetLength() const {
    return m_withTargetLength;
  }
private:
  bool m_withTargetLength;
};

} // namespace Moses

#endif
