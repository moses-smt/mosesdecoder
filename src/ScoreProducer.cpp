// $Id$

#include <iostream>
#include <typeinfo>
#include "ScoreProducer.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"

unsigned int ScoreProducer::s_globalScoreBookkeepingIdCounter(0);

ScoreProducer::~ScoreProducer() {}

ScoreProducer::ScoreProducer()
{
  m_scoreBookkeepingId = s_globalScoreBookkeepingIdCounter++;
  std::cerr << "ScoreProducer created (id=" << m_scoreBookkeepingId << ", this=" << this << ")\n";
}
 
