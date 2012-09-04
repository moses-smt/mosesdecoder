#include "SpanLengthEstimator.h"
#include <vector>
#include <map>

using namespace std;
namespace Moses
{

//Assumes that the scores are passed into order
void SpanLengthEstimator::AddSourceSpanProbas(map<unsigned,float> sourceProbas)
{
        m_sourceScores.push_back(sourceProbas);
}

//Assumes that the scores are passed into order
void SpanLengthEstimator::AddTargetSpanProbas(map<unsigned,float> targetProbas)
{
    m_targetScores.push_back(targetProbas);
}

float SpanLengthEstimator::GetSourceLengthProbas(unsigned nonTerminal, unsigned spanLength) const
{
  map<unsigned,float>::const_iterator it;
  it=m_sourceScores[nonTerminal].find(spanLength);
  return (*it).second;
}

float SpanLengthEstimator::GetTargetLengthProbas(unsigned nonTerminal, unsigned spanLength) const
{
     map<unsigned,float>::const_iterator it;
    it=m_targetScores[nonTerminal].find(spanLength);
    return (*it).second;
}

}
