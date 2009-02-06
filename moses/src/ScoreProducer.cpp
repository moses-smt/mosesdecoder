// $Id$

#include <iostream>
#include <typeinfo>
#include "ScoreProducer.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"

namespace Moses
{
unsigned int ScoreProducer::s_globalScoreBookkeepingIdCounter(0);

ScoreProducer::~ScoreProducer() {}

ScoreProducer::ScoreProducer()
{
  m_scoreBookkeepingId = UNASSIGNED;
}

}

