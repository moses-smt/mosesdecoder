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
  virtual float GetScoreBySpanLength(unsigned spanLength) const = 0;
  virtual void FinishedAdds(unsigned ruleCount) {}
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
