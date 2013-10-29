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

FeatureFunction::
FeatureFunction(const std::string& description,
                const std::string& line)
  : m_tuneable(true)
  , m_numScoreComponents(1)
{
  Initialize(line);
}

FeatureFunction::
FeatureFunction(const std::string& description,
                size_t numScoreComponents,
                const std::string& line)
  : m_tuneable(true)
  , m_numScoreComponents(numScoreComponents)
{
  Initialize(line);
}

void
FeatureFunction::
Initialize(const std::string &line)
{
  ParseLine(line);

  ScoreComponentCollection::RegisterScoreProducer(this);
  m_producers.push_back(this);
}

FeatureFunction::~FeatureFunction() {}

void FeatureFunction::ParseLine(const std::string &line)
{
  vector<string> toks = Tokenize(line);
  CHECK(toks.size());

  string nameStub = toks[0];

  set<string> keys;

  for (size_t i = 1; i < toks.size(); ++i) {
    vector<string> args = TokenizeFirstOnly(toks[i], "=");
    CHECK(args.size() == 2);

    pair<set<string>::iterator,bool> ret = keys.insert(args[0]);
    UTIL_THROW_IF(!ret.second, util::Exception, "Duplicate key in line " << line);

    if (args[0] == "num-features") {
      m_numScoreComponents = Scan<size_t>(args[1]);
    } else if (args[0] == "name") {
      m_description = args[1];
    } else {
      m_args.push_back(args);
    }
  }

  // name
  if (m_description == "") {
    size_t index = description_counts.count(nameStub);

    ostringstream dstream;
    dstream << nameStub;
    dstream << index;

    description_counts.insert(nameStub);
    m_description = dstream.str();
  }

}

void FeatureFunction::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "tuneable") {
    m_tuneable = Scan<bool>(value);
  } else if (key == "filterable") { //ignore
  } else {
    UTIL_THROW(util::Exception, "Unknown argument " << key << "=" << value);
  }
}

void FeatureFunction::ReadParameters()
{
  while (!m_args.empty()) {
    const vector<string> &args = m_args[0];
    SetParameter(args[0], args[1]);

    m_args.erase(m_args.begin());
  }
}

std::vector<float> FeatureFunction::DefaultWeights() const
{
  UTIL_THROW(util::Exception, "No default weights");
}

}

