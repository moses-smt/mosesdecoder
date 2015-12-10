#include <vector>
#include "DeleteRules.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/InputFileStream.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
DeleteRules::DeleteRules(const std::string &line)
  :StatelessFeatureFunction(1, line)
{
  m_tuneable = false;
  ReadParameters();
}

void DeleteRules::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  std::vector<FactorType> factorOrder;
  factorOrder.push_back(0); // unfactored for now

  InputFileStream strme(m_path);

  string line;
  while (getline(strme, line)) {
    vector<string> toks = TokenizeMultiCharSeparator(line, "|||");
    UTIL_THROW_IF2(toks.size() != 2, "Line must be source ||| target");
    Phrase source, target;
    source.CreateFromString(Input, factorOrder, toks[0], NULL);
    target.CreateFromString(Output, factorOrder, toks[1], NULL);

    size_t hash = 0;
    boost::hash_combine(hash, source);
    boost::hash_combine(hash, target);
    m_ruleHashes.insert(hash);
  }
}

void DeleteRules::EvaluateInIsolation(const Phrase &source
                                      , const TargetPhrase &target
                                      , ScoreComponentCollection &scoreBreakdown
                                      , ScoreComponentCollection &estimatedScores) const
{
  // dense scores
  size_t hash = 0;
  boost::hash_combine(hash, source);
  boost::hash_combine(hash, target);

  boost::unordered_set<size_t>::const_iterator iter;
  iter = m_ruleHashes.find(hash);
  if (iter != m_ruleHashes.end()) {
    scoreBreakdown.PlusEquals(this, -std::numeric_limits<float>::infinity());
  }

}

void DeleteRules::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{}

void DeleteRules::EvaluateTranslationOptionListWithSourceContext(const InputType &input

    , const TranslationOptionList &translationOptionList) const
{}

void DeleteRules::EvaluateWhenApplied(const Hypothesis& hypo,
                                      ScoreComponentCollection* accumulator) const
{}

void DeleteRules::EvaluateWhenApplied(const ChartHypothesis &hypo,
                                      ScoreComponentCollection* accumulator) const
{}

void DeleteRules::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

