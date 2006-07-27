#include <iostream>
#include <typeinfo>
#include "ScoreProducer.h"
#include "ScoreIndexManager.h"

#include "PhraseDictionaryTree.h"

unsigned int ScoreProducer::s_globalScoreBookkeepingIdCounter(0);

ScoreProducer::~ScoreProducer() {}

ScoreProducer::ScoreProducer()
{
//  if (dynamic_cast<PhraseDictionaryTree *>(this)) {
//    std::cerr << "Skipping PhraseDictionaryTree\n";
//    return;
//  }
  m_scoreBookkeepingId = ++s_globalScoreBookkeepingIdCounter;
  std::cerr << "ScoreProducer created (id=" << m_scoreBookkeepingId << ", this=" << this << ", type=" << typeid(this).name() << ")\n";
}
 
