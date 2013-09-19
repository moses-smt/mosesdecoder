// $Id: ScoreProducer.cpp,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $

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

