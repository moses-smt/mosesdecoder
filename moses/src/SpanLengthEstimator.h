#ifndef moses_SpanLengthEstimator_h
#define moses_SpanLengthEstimator_h

#include <vector>
#include <map>

namespace Moses
{

class SpanLengthEstimator
{
public:
  virtual void AddSpanScore(unsigned spanLength, float score) = 0;
  virtual void AddSpanScore_ISI(float count, unsigned sum_len, unsigned sum_square_len){};
  //virtual float PDFWithPriorMean(unsigned spanLength, unsigned count, float prior_mean, float prior_variance){return 1.0;};
  virtual void PosteriorParameters(float count, float prior_mean, float prior_variance){};
  virtual float GetScoreBySpanLength(unsigned spanLength) const = 0;
  virtual void FinishedAdds(float ruleCount) {}
  virtual ~SpanLengthEstimator() {}
};
  
SpanLengthEstimator* CreateAsIsSpanLengthEstimator();
SpanLengthEstimator* CreateGaussianSpanLengthEstimator();
  
class SpanLengthEstimatorCollection
{
  typedef std::vector<SpanLengthEstimator*> TEstimatorList;
  TEstimatorList m_sourceEstimators, m_targetEstimators;
public:
  template<class Iter>
  void AssignSource(Iter begin, Iter end) {
    m_sourceEstimators.assign(begin, end);
  }
  template<class Iter>
  void AssignTarget(Iter begin, Iter end) {
    m_targetEstimators.assign(begin, end);
  }
  ~SpanLengthEstimatorCollection();
  float GetScoreBySourceSpanLength(unsigned nonTerminalIndex, unsigned spanLength) const;
  float GetScoreByTargetSpanLength(unsigned nonTerminalIndex, unsigned spanLength) const;
};

}//namespace
#endif
