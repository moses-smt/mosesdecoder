// $Id$

#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Moses
{
ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance().GetTotalScoreComponents(0), 0.0f)
  , m_sim(&StaticData::Instance().GetScoreIndexManager(0))
{}

ScoreComponentCollection::ScoreComponentCollection(int id)
	: m_scores(StaticData::Instance().GetTotalScoreComponents(id), 0.0f)
	, m_sim(&StaticData::Instance().GetScoreIndexManager(id))
	{}
}


