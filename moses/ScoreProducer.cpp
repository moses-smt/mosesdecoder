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

ScoreProducer::ScoreProducer(const std::string& description, const std::string &line)
: m_reportSparseFeatures(false)
{
  ParseLine(line);
  m_numScoreComponents = FindNumFeatures();
  size_t index = description_counts.count(description);

  ostringstream dstream;
  dstream << description;
  dstream << index;

  description_counts.insert(description);

  m_description = dstream.str();
  if (m_numScoreComponents != unlimited)
  {
    ScoreComponentCollection::RegisterScoreProducer(this);
  }
}

ScoreProducer::ScoreProducer(const std::string& description, size_t numScoreComponents, const std::string &line)
  : m_reportSparseFeatures(false), m_numScoreComponents(numScoreComponents)
{
  ParseLine(line);
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

void ScoreProducer::ParseLine(const std::string &line)
{
  cerr << "line=" << line << endl;
  vector<string> toks = Tokenize(line);
  for (size_t i = 1; i < toks.size(); ++i) {
    vector<string> args = Tokenize(toks[i], "=");
    //CHECK(args.size() == 2);
    m_args.push_back(args);
  }
}

size_t ScoreProducer::FindNumFeatures()
{
  for (size_t i = 0; i < m_args.size(); ++i) {
    if (m_args[i][0] == "num-features") {
      size_t ret = Scan<size_t>(m_args[i][1]);
      m_args.erase(m_args.begin() + i);
      return ret;
    }
  }
  abort();
}
} // namespace


