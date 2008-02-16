// $Id: ScoreComponentCollection.cpp 87 2007-09-10 23:17:43Z hieu $

#include "ScoreComponentCollection.h"
#include "StaticData.h"
#include "Util.h"

ScoreComponentCollection::ScoreComponentCollection()
  : m_scores(StaticData::Instance().GetTotalScoreComponents(), 0.0f)
  , m_sim(&StaticData::Instance().GetScoreIndexManager())
{}
