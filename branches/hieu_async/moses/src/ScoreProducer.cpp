// $Id: ScoreProducer.cpp 87 2007-09-10 23:17:43Z hieu $

#include <iostream>
#include <typeinfo>
#include "ScoreProducer.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"

unsigned int ScoreProducer::s_globalScoreBookkeepingIdCounter(0);

ScoreProducer::~ScoreProducer() {}

ScoreProducer::ScoreProducer()
{
  m_scoreBookkeepingId = UNASSIGNED;
}
 
