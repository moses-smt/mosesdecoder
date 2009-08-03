// $Id$

#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Moses
{
ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance().GetTotalScoreComponents(), 0.0f)
  , m_sim(&StaticData::Instance().GetScoreIndexManager())
{}

  //!Calc L1 Norm
  float ScoreComponentCollection::GetL1Norm() const {
    
    float norm = 0.0;
    for (std::vector<float>::const_iterator vi = m_scores.begin();
		     vi != m_scores.end(); ++vi)
		{
      float score = *vi;
      score = abs(score);
			norm += score;
		}
    return norm;
  }
  
  //!Calc L2 Norm
  float ScoreComponentCollection::GetL2Norm() const {
    
    float norm = 0.0;
    for (std::vector<float>::const_iterator vi = m_scores.begin();
		     vi != m_scores.end(); ++vi)
		{
			norm += *vi * *vi;
		}
    return sqrt(norm);
  }  
  
}


