// $Id$

#include <iostream>
#include <typeinfo>
#include "ScoreProducer.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"

namespace Moses
{
  
#ifdef WITH_THREADS
std::map<boost::thread::id, unsigned int> ScoreProducer::s_globalScoreBookkeepingIdCounterThreadMap;
#else
unsigned int ScoreProducer::s_globalScoreBookkeepingIdCounter(0);
#endif

ScoreProducer::~ScoreProducer() {}

ScoreProducer::ScoreProducer()
{
  m_scoreBookkeepingId = UNASSIGNED;
}

}

