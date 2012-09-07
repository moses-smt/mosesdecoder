#include "SpanLengthEstimator.h"
#include <vector>
#include <map>
#include "util/check.hh"
#include "StaticData.h"
#include "TypeDef.h"

using namespace std;
namespace Moses
{

class AsIsSpanEstimator: public SpanLengthEstimator
{
  typedef std::map<unsigned, float> TLengthToScoreMap;
  TLengthToScoreMap m_scores;
public:    
  virtual void AddSpanScore(unsigned spanLength, float score) {
    m_scores.insert(make_pair(spanLength, score));
  }
  virtual float GetScoreBySpanLength(unsigned spanLength) const {
    // bool useGaussian = StaticData::Instance().GetParam("gaussian-span-length-score").size() > 0;
    if (m_scores.empty())
      return 0.0;
    TLengthToScoreMap::const_iterator iter = m_scores.find(spanLength);
    if (iter == m_scores.end())
      return LOWEST_SCORE;
    else
      return iter->second;
  }
};
    
SpanLengthEstimator* CreateAsIsSpanLengthEstimator() {
  return new AsIsSpanEstimator();
}
  
  class GaussianSpanLengthEstimator : public SpanLengthEstimator
  {
    float m_average, m_averageSquare;
    float m_logSqrt2Pi;
    float m_sigma, m_logSigma;
  public:
    GaussianSpanLengthEstimator()
    : m_average(0.0)
    , m_averageSquare(0.0)
    , m_logSqrt2Pi(0.5*log(2*acos(-1.0)))
    , m_sigma(0.0), m_logSigma(0.0)
    {}
    
    virtual void AddSpanScore(unsigned spanLength, float score) {
      m_average += exp(score) * spanLength;
      m_averageSquare += exp(score) * spanLength * spanLength;
    }
    virtual float GetScoreBySpanLength(unsigned spanLength) const {
      float t = ((spanLength - m_average) / m_sigma);
      return -m_logSqrt2Pi - m_logSigma - 0.5 * t * t;
    }
    virtual void FinishedAdds() {
      m_sigma = sqrt(m_averageSquare - m_average*m_average);
      m_logSigma = log(max(1.0f, m_sigma));
    }
  };
  
  SpanLengthEstimator* CreateGaussianSpanLengthEstimator()
  {
    return new GaussianSpanLengthEstimator();
  }
  
float SpanLengthEstimatorCollection::GetScoreBySourceSpanLength(
  unsigned nonTerminalIndex,
  unsigned sourceSpanLength) const
{
  if (m_sourceEstimators.empty())
    return 0.0f;
  CHECK(m_sourceEstimators.size() > size_t(nonTerminalIndex));
  return m_sourceEstimators[nonTerminalIndex]->GetScoreBySpanLength(sourceSpanLength);
}
  
float SpanLengthEstimatorCollection::GetScoreByTargetSpanLength(
  unsigned nonTerminalIndex,
  unsigned targetSpanLength) const
{
  if (m_targetEstimators.empty())
    return 0.0f;
  CHECK(m_targetEstimators.size() > size_t(nonTerminalIndex));
  return m_targetEstimators[nonTerminalIndex]->GetScoreBySpanLength(targetSpanLength);
}

SpanLengthEstimatorCollection::~SpanLengthEstimatorCollection()
{
  RemoveAllInColl(m_targetEstimators);
  RemoveAllInColl(m_sourceEstimators);
}

} // namespace
