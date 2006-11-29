// $Id$

#include "ScoreComponentCollection.h"
#include "StaticData.h"
#include "Util.h"

ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance()->GetTotalScoreComponents(), 0.0f)
  , m_sim(&StaticData::Instance()->GetScoreIndexManager())
{}

void ScoreComponentCollection::FloorAll()
{
	vector<float>::iterator iter;
	for (iter = m_scores.begin() ; iter != m_scores.end() ; ++iter)
	{
		(*iter) = FloorScore(-numeric_limits<float>::infinity());
	}
}
