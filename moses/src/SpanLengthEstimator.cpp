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
  float min;
  unsigned m_ISIk;
public:
  GaussianSpanLengthEstimator()
  : m_average(0.0)
  , m_averageSquare(0.0)
  , m_logSqrt2Pi(0.5*log(2*acos(-1.0)))
  , m_sigma(0.0), m_logSigma(0.0),min(-20*log(10)),m_ISIk(10)
  {}
  
  virtual void AddSpanScore(unsigned spanLength, float score) {
    m_average += exp(score) * spanLength;
    m_averageSquare += exp(score) * spanLength * spanLength;
  }

  virtual void AddSpanScore_ISI(unsigned count, float sum_len, float sum_square_len){
    CHECK(count>0);
    m_average = sum_len / count;
    m_averageSquare = sum_square_len /count;
  }
  
  virtual float GetScoreBySpanLength(unsigned spanLength) const {
    float t = ((spanLength - m_average) / m_sigma);
    float ret = -m_logSqrt2Pi - m_logSigma - 0.5 * t * t;
    return max(ret,min);
  }
  //MARIA 
  //modified variance to match ISI formula; ruleCount is extracted from rule table: tokens[4] -> first count (totalCount)
  virtual void FinishedAdds(unsigned ruleCount) {
    //m_sigma = max(1.0f, sqrt(m_averageSquare - m_average*m_average));
    CHECK(ruleCount > 0);
    m_sigma=max(1.0f,sqrt(m_averageSquare - m_average*m_average+m_ISIk/(1+m_ISIk/ruleCount)*m_averageSquare));
    m_logSigma = log(m_sigma);
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
