#include <stdexcept>

#include "util/check.hh"

#include "FeatureFunction.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/TranslationOption.h"

using namespace std;

namespace Moses
{

multiset<string> FeatureFunction::description_counts;

std::vector<FeatureFunction*> FeatureFunction::m_producers;
std::vector<const StatelessFeatureFunction*> StatelessFeatureFunction::m_statelessFFs;
std::vector<const StatefulFeatureFunction*>  StatefulFeatureFunction::m_statefulFFs;

FeatureFunction::FeatureFunction(const std::string& description, const std::string &line)
: m_tuneable(true)
{
  ParseLine(description, line);

  if (m_description == "") {
    // not been given a name. Make a unique name
    size_t index = description_counts.count(description);

    ostringstream dstream;
    dstream << description;
    dstream << index;

    description_counts.insert(description);
    m_description = dstream.str();
  }

	ScoreComponentCollection::RegisterScoreProducer(this);
  m_producers.push_back(this);
}

FeatureFunction::FeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line)
: m_numScoreComponents(numScoreComponents)
, m_tuneable(true)
{
  ParseLine(description, line);

  if (m_description == "") {
    size_t index = description_counts.count(description);

    ostringstream dstream;
    dstream << description;
    dstream << index;

    description_counts.insert(description);
    m_description = dstream.str();
  }

  ScoreComponentCollection::RegisterScoreProducer(this);
  m_producers.push_back(this);
}

FeatureFunction::~FeatureFunction() {}

void FeatureFunction::ParseLine(const std::string& description, const std::string &line)
{
  vector<string> toks = Tokenize(line);

  CHECK(toks.size());
  //CHECK(toks[0] == description);

  for (size_t i = 1; i < toks.size(); ++i) {
    vector<string> args = Tokenize(toks[i], "=");
    CHECK(args.size() == 2);

    if (args[0] == "num-features") {
      m_numScoreComponents = Scan<size_t>(args[1]);
    }
    else if (args[0] == "name") {
      m_description = args[1];
    }
    else if (args[0] == "tuneable") {
      m_tuneable = Scan<bool>(args[1]);
    }
    else {
      m_args.push_back(args);
    }
  }
}

}

