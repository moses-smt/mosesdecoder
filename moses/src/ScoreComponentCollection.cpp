// $Id: ScoreComponentCollection.cpp 2282 2009-04-15 11:04:27Z hieuhoang1972 $

#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Moses
{
ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance().GetTotalScoreComponents(), 0.0f)
  , m_sim(&StaticData::Instance().GetScoreIndexManager())
{}

float ScoreComponentCollection::GetWeightedScore() const
{
	float ret = InnerProduct(StaticData::Instance().GetAllWeights());
	return ret;
}

void ScoreComponentCollection::ZeroAllLM()
{
	const LMList &lmList = StaticData::Instance().GetAllLM();
	
	for (size_t ind = lmList.GetMinIndex(); ind <= lmList.GetMaxIndex(); ++ind)
	{
		m_scores[ind] = 0;
	}
}

void ScoreComponentCollection::PlusEqualsAllLM(const ScoreComponentCollection& rhs)
{
	const LMList &lmList = StaticData::Instance().GetAllLM();
	
	for (size_t ind = lmList.GetMinIndex(); ind <= lmList.GetMaxIndex(); ++ind)
	{
		m_scores[ind] += rhs.m_scores[ind];
	}

}

}


