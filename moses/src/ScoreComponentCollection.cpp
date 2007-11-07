// $Id$

#include "ScoreComponentCollection.h"
#include "StaticData.h"

ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance().GetTotalScoreComponents(), 0.0f)
  , m_sim(&StaticData::Instance().GetScoreIndexManager())
{}

float ScoreComponentCollection::CombineScoreBreakdown()
{
	vector<float>::iterator scores_iter;
		float combinedScore = 0.0;
		for(scores_iter = m_scores.begin(); scores_iter != m_scores.end(); ++scores_iter)
		{
			float f = *scores_iter;
			// combine the scores of scoreBreakdown by just summing them up
			// TODO: weighted combination
			combinedScore += f;
		}
		return combinedScore;
}

