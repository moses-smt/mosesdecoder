#include "RuleScope.h"
#include "moses/StaticData.h"
#include "moses/Word.h"

namespace Moses
{
RuleScope::RuleScope(const std::string &line)
  :StatelessFeatureFunction(1, line)
  ,m_sourceSyntax(true)
{
}

bool IsAmbiguous(const Word &word, bool sourceSyntax)
{
  const Word &inputDefaultNonTerminal = StaticData::Instance().GetInputDefaultNonTerminal();
  return word.IsNonTerminal() && (!sourceSyntax || word == inputDefaultNonTerminal);
}

void RuleScope::EvaluateInIsolation(const Phrase &source
                                    , const TargetPhrase &targetPhrase
                                    , ScoreComponentCollection &scoreBreakdown
                                    , ScoreComponentCollection &estimatedFutureScore) const
{
  // adjacent non-term count as 1 ammbiguity, rather than 2 as in rule scope
  // source can't be empty, right?
  float score = 0;

  int count = 0;
  for (size_t i = 0; i < source.GetSize() - 0; ++i) {
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

  scoreBreakdown.PlusEquals(this, score);
}

void RuleScope::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "source-syntax") {
    m_sourceSyntax = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

