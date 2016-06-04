#include <sstream>
#include "EditOps.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TranslationOption.h"
#include "util/string_piece_hash.hh"
#include "util/exception.hh"

#include <functional>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "Diffs.h"

namespace Moses
{

using namespace std;

std::string ParseScores(const std::string &line, const std::string& defaultScores)
{
  std::vector<std::string> toks = Tokenize(line);
  UTIL_THROW_IF2(toks.empty(), "Empty line");

  for (size_t i = 1; i < toks.size(); ++i) {
    std::vector<std::string> args = TokenizeFirstOnly(toks[i], "=");
    UTIL_THROW_IF2(args.size() != 2,
                   "Incorrect format for feature function arg: " << toks[i]);

    if (args[0] == "scores") {
      return args[1];
    }
  }
  return defaultScores;
}

EditOps::EditOps(const std::string &line)
  : StatelessFeatureFunction(ParseScores(line, "dis").size(), line)
  , m_factorType(0), m_chars(false), m_scores(ParseScores(line, "dis"))
{
  std::cerr << "Initializing EditOps feature.." << std::endl;
  ReadParameters();
}

void EditOps::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else if (key == "chars") {
    m_chars = Scan<bool>(value);
  } else if (key == "scores") {
    m_scores = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void EditOps::Load()
{ }

void EditOps::EvaluateInIsolation(const Phrase &source
                                  , const TargetPhrase &target
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection &estimatedFutureScore) const
{
  ComputeFeatures(source, target, &scoreBreakdown);
}

void EditOps::ComputeFeatures(
  const Phrase &source,
  const TargetPhrase& target,
  ScoreComponentCollection* accumulator) const
{
  std::vector<float> ops(GetNumScoreComponents(), 0);

  if(m_chars) {
    std::vector<FactorType> factors;
    factors.push_back(m_factorType);

    std::string sourceStr = source.GetStringRep(factors);
    std::string targetStr = target.GetStringRep(factors);

    AddStats(sourceStr, targetStr, m_scores, ops);
  } else {
    std::vector<std::string> sourceTokens;
    //std::cerr << "Ed src: ";
    for(size_t i = 0; i < source.GetSize(); ++i) {
      if(!source.GetWord(i).IsNonTerminal())
        sourceTokens.push_back(source.GetWord(i).GetFactor(m_factorType)->GetString().as_string());
      //std::cerr << sourceTokens.back() << " ";
    }
    //std::cerr << std::endl;

    std::vector<std::string> targetTokens;
    //std::cerr << "Ed trg: ";
    for(size_t i = 0; i < target.GetSize(); ++i) {
      if(!target.GetWord(i).IsNonTerminal())
        targetTokens.push_back(target.GetWord(i).GetFactor(m_factorType)->GetString().as_string());
      //std::cerr << targetTokens.back() << " ";
    }
    //std::cerr << std::endl;

    AddStats(sourceTokens, targetTokens, m_scores, ops);
  }

  accumulator->PlusEquals(this, ops);
}

bool EditOps::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_factorType];
  return ret;
}

}
