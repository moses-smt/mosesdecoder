// $Id$

#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Moses
{
ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(0.0f)
  , m_sim(&StaticData::Instance().GetScoreIndexManager())
{}

float ScoreComponentCollection::GetWeightedScore() const
{
	float ret = InnerProduct(StaticData::Instance().GetAllWeights());
	return ret;
}
		
void ScoreComponentCollection::ZeroAllLM(const LMList& lmList)
{
	
	for (size_t ind = lmList.GetMinIndex(); ind <= lmList.GetMaxIndex(); ++ind)
	{
		m_scores[m_sim->GetFeatureName(ind)] = 0;
	}
}

void ScoreComponentCollection::PlusEqualsAllLM(const LMList& lmList, const ScoreComponentCollection& rhs)
{
	
	for (size_t ind = lmList.GetMinIndex(); ind <= lmList.GetMaxIndex(); ++ind)
	{
		m_scores[m_sim->GetFeatureName(ind)] += rhs.m_scores[m_sim->GetFeatureName(ind)];
	}
	
}
	
}


