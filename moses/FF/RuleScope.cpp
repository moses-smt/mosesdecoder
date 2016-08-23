#include "RuleScope.h"
#include "moses/StaticData.h"
#include "moses/Word.h"

using namespace std;

namespace Moses
{
RuleScope::RuleScope(const std::string &line)
  :StatelessFeatureFunction(1, line)
  ,m_sourceSyntax(true)
  ,m_perScope(false)
  ,m_futureCostOnly(false)
{
}

// bool IsAmbiguous(const Word &word, bool sourceSyntax)
// {
//   const Word &inputDefaultNonTerminal = StaticData::Instance().GetInputDefaultNonTerminal();
//   return word.IsNonTerminal() && (!sourceSyntax || word == inputDefaultNonTerminal);
// }

void RuleScope::EvaluateInIsolation(const Phrase &source
                                    , const TargetPhrase &targetPhrase
                                    , ScoreComponentCollection &scoreBreakdown
                                    , ScoreComponentCollection &estimatedScores) const
{
  if (IsGlueRule(source)) {
    return;
  }

  float score = 0;

  if (source.GetSize() > 0 && source.Front().IsNonTerminal()) {
    ++score;
  }
  if (source.GetSize() > 1 && source.Back().IsNonTerminal()) {
    ++score;
  }

  /*
  int count = 0;
  for (size_t i = 0; i < source.GetSize(); ++i) {
    const Word &word = source.GetWord(i);
    bool ambiguous = IsAmbiguous(word, m_sourceSyntax);
    if (ambiguous) {
      ++count;
    } else {
      if (count > 0) {
        score += count;
      }
      count = -1;
    }
  }

  // 1st & last always adjacent to ambiguity
  ++count;
  if (count > 0) {
    score += count;
  }
  */

  if (m_perScope) {
    UTIL_THROW_IF2(m_numScoreComponents <= score,
                   "Insufficient number of score components. Scope=" << score << ". NUmber of score components=" << score);
    vector<float> scores(m_numScoreComponents, 0);
    scores[score] = 1;

    if (m_futureCostOnly) {
      estimatedScores.PlusEquals(this, scores);
    } else {
      scoreBreakdown.PlusEquals(this, scores);
    }
  } else if (m_futureCostOnly) {
    estimatedScores.PlusEquals(this, score);
  } else {
    scoreBreakdown.PlusEquals(this, score);
  }
}

void RuleScope::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "source-syntax") {
    m_sourceSyntax = Scan<bool>(value);
  } else if (key == "per-scope") {
    m_perScope = Scan<bool>(value);
  } else if ("future-cost-only") {
    m_futureCostOnly = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

bool RuleScope::IsGlueRule(const Phrase &source) const
{
  string sourceStr = source.ToString();
  if (sourceStr == "<s> " || sourceStr == "X </s> " || sourceStr == "X X ") {
    // don't score glue rule
    //cerr << "sourceStr=" << sourceStr << endl;
    return true;
  } else {
    return false;
  }

}

}

