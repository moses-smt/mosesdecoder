#include <stdexcept>

#include "util/check.hh"
#include "util/exception.hh"

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

FeatureFunction &FeatureFunction::FindFeatureFunction(const std::string& name)
{
  for (size_t i = 0; i < m_producers.size(); ++i) {
    FeatureFunction &ff = *m_producers[i];
    if (ff.GetScoreProducerDescription() == name) {
      return ff;
    }
  }

  throw "Unknown feature " + name;
}

FeatureFunction::FeatureFunction(const std::string& description, const std::string &line)
  : m_tuneable(true)
  , m_numScoreComponents(1)
{
  Initialize(description, line);
}

FeatureFunction::FeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line)
  : m_numScoreComponents(numScoreComponents)
  , m_tuneable(true)
{
  Initialize(description, line);
}

void FeatureFunction::Initialize(const std::string& description, const std::string &line)
{
  ParseLine(description, line);

  size_t ind = 0;
  while (ind < m_args.size()) {
    vector<string> &args = m_args[ind];
    bool consumed = SetParameter(args[0], args[1]);
    if (consumed) {
      m_args.erase(m_args.begin() + ind);
    } else {
      ++ind;
    }
  }

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

  set<string> keys;

  for (size_t i = 1; i < toks.size(); ++i) {
    vector<string> args = Tokenize(toks[i], "=");
    CHECK(args.size() == 2);

    pair<set<string>::iterator,bool> ret = keys.insert(args[0]);
    UTIL_THROW_IF(!ret.second, util::Exception, "Duplicate key in line " << line);

    m_args.push_back(args);
  }
}

bool FeatureFunction::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "num-features") {
    m_numScoreComponents = Scan<size_t>(value);
  } else if (key == "name") {
    m_description = value;
  } else if (key == "tuneable") {
    m_tuneable = Scan<bool>(value);
  } else {
    return false;
  }

  return true;
}

void FeatureFunction::OverrideParameter(const std::string& key, const std::string& value)
{
  bool ret = SetParameter(key, value);
  UTIL_THROW_IF(!ret, util::Exception, "Unknown argument" << key);
}

}

