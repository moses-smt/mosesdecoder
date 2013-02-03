#include <stdexcept>

#include "util/check.hh"

#include "ChartHypothesis.h"
#include "ChartManager.h"
#include "FeatureFunction.h"
#include "Hypothesis.h"
#include "Manager.h"
#include "TranslationOption.h"


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

std::vector<FeatureFunction*> FeatureFunction::m_producers;
std::vector<const StatelessFeatureFunction*> StatelessFeatureFunction::m_statelessFFs;
std::vector<const StatefulFeatureFunction*>  StatefulFeatureFunction::m_statefulFFs;

FeatureFunction::FeatureFunction(const std::string& description, const std::string &line)
: ScoreProducer(description, line)
{
  m_producers.push_back(this);
}

FeatureFunction::FeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line)
: ScoreProducer(description, numScoreComponents, line)
{
  m_producers.push_back(this);
}


FeatureFunction::~FeatureFunction() {}

StatelessFeatureFunction::StatelessFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line)
:FeatureFunction(description, numScoreComponents, line)
{
  m_statelessFFs.push_back(this);
}

bool StatelessFeatureFunction::IsStateless() const
{
  return true;
}

bool StatelessFeatureFunction::ComputeValueInTranslationOption() const
{
  return false;
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

