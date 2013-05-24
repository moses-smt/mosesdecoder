#include <stdexcept>

#include "util/check.hh"

#include "FeatureFunction.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/TranslationOption.h"

using namespace std;

namespace Moses
{

PhraseBasedFeatureContext::PhraseBasedFeatureContext(const Hypothesis* hypothesis) :
  m_hypothesis(hypothesis),
  m_translationOption(m_hypothesis->GetTranslationOption()),
  m_source(m_hypothesis->GetManager().GetSource()) {}

PhraseBasedFeatureContext::PhraseBasedFeatureContext
      (const TranslationOption& translationOption, const InputType& source) :
  m_hypothesis(NULL),
  m_translationOption(translationOption),
  m_source(source) {}

const TranslationOption& PhraseBasedFeatureContext::GetTranslationOption() const
{
  return m_translationOption;
}

const InputType& PhraseBasedFeatureContext::GetSource() const
{
  return m_source;
}

const TargetPhrase& PhraseBasedFeatureContext::GetTargetPhrase() const
{
  return m_translationOption.GetTargetPhrase();
}

const WordsBitmap& PhraseBasedFeatureContext::GetWordsBitmap() const
{
  if (!m_hypothesis) {
    throw std::logic_error("Coverage vector not available during pre-calculation");
  }
  return m_hypothesis->GetWordsBitmap();
}


ChartBasedFeatureContext::ChartBasedFeatureContext
                        (const ChartHypothesis* hypothesis):
  m_hypothesis(hypothesis),
  m_targetPhrase(hypothesis->GetCurrTargetPhrase()),
  m_source(hypothesis->GetManager().GetSource()) {}

ChartBasedFeatureContext::ChartBasedFeatureContext(
                         const TargetPhrase& targetPhrase,
                         const InputType& source):
  m_hypothesis(NULL),
  m_targetPhrase(targetPhrase),
  m_source(source) {}

const InputType& ChartBasedFeatureContext::GetSource() const
{
  return m_source;
}

const TargetPhrase& ChartBasedFeatureContext::GetTargetPhrase() const
{
  return m_targetPhrase;
}

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

void FeatureFunction::Evaluate(const InputType &source
                    , ScoreComponentCollection &scoreBreakdown) const
{

}

StatelessFeatureFunction::StatelessFeatureFunction(const std::string& description, const std::string &line)
:FeatureFunction(description, line)
{
  m_statelessFFs.push_back(this);
}

StatelessFeatureFunction::StatelessFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line)
:FeatureFunction(description, numScoreComponents, line)
{
  m_statelessFFs.push_back(this);
}

StatefulFeatureFunction::StatefulFeatureFunction(const std::string& description, const std::string &line)
: FeatureFunction(description, line)
{
  m_statefulFFs.push_back(this);
}

StatefulFeatureFunction::StatefulFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line)
: FeatureFunction(description,numScoreComponents, line)
{
  m_statefulFFs.push_back(this);
}

bool StatefulFeatureFunction::IsStateless() const
{
  return false;
}

}

