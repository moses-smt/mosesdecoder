// $Id$

#include <iostream>
#include <sstream>

#include "ScoreComponentCollection.h"
#include "ScoreProducer.h"

using namespace std;

namespace Moses
{

multiset<string> ScoreProducer::description_counts;
const size_t ScoreProducer::unlimited = -1;

ScoreProducer::ScoreProducer(const std::string& description, size_t numScoreComponents)
  : m_reportSparseFeatures(false), m_numScoreComponents(numScoreComponents)
{
  description_counts.insert(description);
  size_t count = description_counts.count(description);
  ostringstream dstream;
  dstream << description;
  if (count > 1) 
  {
    dstream << ":" << count;
  }
  m_description = dstream.str();
  if (numScoreComponents != unlimited)
  {
    ScoreComponentCollection::RegisterScoreProducer(this);
  }
}

ScoreProducer::~ScoreProducer() {
}

}

