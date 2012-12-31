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
  size_t index = description_counts.count(description);

  ostringstream dstream;
  dstream << description;
  dstream << index;

  description_counts.insert(description);

  m_description = dstream.str();
  if (numScoreComponents != unlimited)
  {
    ScoreComponentCollection::RegisterScoreProducer(this);
  }
}

ScoreProducer::~ScoreProducer()
{
  cerr << endl << "In ~ScoreProducer of" << this << endl;
}

}


