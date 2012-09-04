#include "SpanLengthEstimator.h"
#include <vector>
#include <map>

using namespace std;
namespace Moses
{

//Assumes that the scores are passed into order
void SpanLengthEstimator::AddSourceSpanProbas(map<size_t,float> sourceProbas)
{
        m_sourceScores.push_back(sourceProbas);
}

//Assumes that the scores are passed into order
void SpanLengthEstimator::AddTargetSpanProbas(map<size_t,float> targetProbas)
{
    m_targetScores.push_back(targetProbas);
}

float SpanLengthEstimator::GetSourceLengthProbas(size_t nonTerminal, size_t spanLength) const
{
  map<size_t,float>::const_iterator it;
  it=m_sourceScores[nonTerminal].find(spanLength);
  return (*it).second;
}

float SpanLengthEstimator::GetTargetLengthProbas(size_t nonTerminal, size_t spanLength) const
{
     map<size_t,float>::const_iterator it;
    it=m_targetScores[nonTerminal].find(spanLength);
    return (*it).second;
}

}
