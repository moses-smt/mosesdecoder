#include "SpanLengthEstimator.h"
#include <vector>
#include <map>
#include "util/check.hh"

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
  if(m_sourceScores.empty())
  {
      return 0.0;
  }
  CHECK(! (nonTerminal > m_sourceScores.size()) );
  it=m_sourceScores[nonTerminal].find(spanLength);
  if(it!=m_sourceScores[nonTerminal].end())
    {return (*it).second;}
    else
    {
      return -1000.0;
    }
}

float SpanLengthEstimator::GetTargetLengthProbas(unsigned nonTerminal, unsigned spanLength) const
{
     CHECK(m_targetScores.size() == 0);
     /*map<unsigned,float>::const_iterator it;
     if(m_targetScores.empty())
    {
        return 0.0;
    }
    if(it!=m_targetScores[nonTerminal].end())
    {return (*it).second;}
    else
    {
      return -1000.0;
    }*/
    return 0.0;
}

}
