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
  bool m_DefPrior;
public:
  GaussianSpanLengthEstimator()
  : m_average(0.0)
  , m_averageSquare(0.0)
  , m_logSqrt2Pi(0.5*log(2*acos(-1.0)))
  , m_sigma(0.0), m_logSigma(0.0),min(-20*log(10)),m_ISIk(10),m_DefPrior(true)
  {}
  
  virtual void AddSpanScore(unsigned spanLength, float score) {
    m_average += exp(score) * spanLength;
    m_averageSquare += exp(score) * spanLength * spanLength;
  }

  virtual void AddSpanScore_ISI(float count, unsigned sum_len, unsigned sum_square_len){
    CHECK(count>0.0);
    m_average = (float)sum_len / count;
    m_averageSquare = (float)sum_square_len /count;
  }
 
//  virtual float PDFWithPriorMean(unsigned spanLength, float count, float prior_mean, float prior_variance)
  virtual void PosteriorParameters(float count, float prior_mean, float prior_variance)
  {
    float posterior_mean=0.0f, posterior_variance=0.0f, sample_variance=0.0f;
    sample_variance=m_averageSquare - m_average*m_average; //without ISI smoothing
    //??//posterior_mean=(sample_variance*prior_mean+count*prior_variance*m_average)/(count*prior_variance+sample_variance);
    sample_variance=max(1.0f,sample_variance);
    posterior_variance=1.0f/((1.0f/prior_variance)+(count/sample_variance));
    posterior_mean=posterior_variance*(prior_mean/prior_variance+count*m_average/sample_variance);
    posterior_variance=posterior_variance+sample_variance;

    m_average=posterior_mean;
    m_sigma=posterior_variance;

//    float t = ((spanLength - posterior_mean) / posterior_variance);
//    float ret = -m_logSqrt2Pi - log(posterior_variance) - 0.5 * t * t;
//    return ret;
  }
	  

  virtual float GetScoreBySpanLength(unsigned spanLength) const {
    float t = ((spanLength - m_average) / m_sigma);
    float ret = -m_logSqrt2Pi - m_logSigma - 0.5 * t * t;
    return max(ret,min);
  }
  //MARIA 
  //modified variance to match ISI formula; ruleCount is extracted from rule table: tokens[4] -> first count (totalCount)
  virtual void FinishedAdds(float ruleCount) {
    if(m_DefPrior){
      float prior_mean=0.0, prior_variance=80.0;
      PosteriorParameters(ruleCount, prior_mean, prior_variance);
    }
    else
      m_sigma = max(1.0f, sqrt(m_averageSquare - m_average*m_average));
    //CHECK(ruleCount > 0.0);
    //m_sigma=max(0.001f,sqrt(m_averageSquare - m_average*m_average+m_ISIk/(1+(float)m_ISIk/ruleCount)*m_averageSquare));
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
