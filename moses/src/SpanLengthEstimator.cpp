#include "SpanLengthEstimator.h"
#include <vector>
#include <map>

using namespace std;
namespace Moses
{

SpanLengthEstimator::SpanLengthEstimator(size_t sourceScores, size_t targetScores)
{
    m_sourceScores.resize(sourceScores);
    m_targetScores.resize(targetScores);
}

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

float SpanLengthEstimator::GetSourceLengthProbas(size_t nonTerminal, size_t spanLength)
{
  map<size_t,float>::iterator it;
  it=m_sourceScores[nonTerminal].find(spanLength);
  return (*it).second;
}

float SpanLengthEstimator::GetTargetLengthProbas(size_t nonTerminal, size_t spanLength)
{
     map<size_t,float>::iterator it;
    it=m_targetScores[nonTerminal].find(spanLength);
    return (*it).second;
}

}
