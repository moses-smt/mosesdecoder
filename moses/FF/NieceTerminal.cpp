#include <vector>
#include <set>
#include "NieceTerminal.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartCellLabel.h"
#include "moses/InputType.h"

using namespace std;

namespace Moses
{
NieceTerminal::NieceTerminal(const std::string &line)
  :StatelessFeatureFunction(line)
  ,m_hardConstraint(false)
{
  ReadParameters();
}

void NieceTerminal::Evaluate(const Phrase &source
                             , const TargetPhrase &targetPhrase
                             , ScoreComponentCollection &scoreBreakdown
                             , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void NieceTerminal::Evaluate(const InputType &input
                             , const InputPath &inputPath
                             , const TargetPhrase &targetPhrase
                             , const StackVec *stackVec
                             , ScoreComponentCollection &scoreBreakdown
                             , ScoreComponentCollection *estimatedFutureScore) const
{
  assert(stackVec);

  const Phrase *ruleSource = targetPhrase.GetRuleSource();
  assert(ruleSource);

  std::set<Word> terms;
  for (size_t i = 0; i < ruleSource->GetSize(); ++i) {
    const Word &word = ruleSource->GetWord(i);
    if (!word.IsNonTerminal()) {
      terms.insert(word);
    }
  }

  for (size_t i = 0; i < stackVec->size(); ++i) {
    const ChartCellLabel &cell = *stackVec->at(i);
    const WordsRange &ntRange = cell.GetCoverage();
    bool containTerm = ContainTerm(input, ntRange, terms);

    if (containTerm) {
      //cerr << "ruleSource=" << *ruleSource << " ";
      //cerr << "ntRange=" << ntRange << endl;

      // non-term contains 1 of the terms in the rule.
      float score = m_hardConstraint ? - std::numeric_limits<float>::infinity() : 1;
      scoreBreakdown.PlusEquals(this, score);
      return;
    }
  }

}

void NieceTerminal::Evaluate(const Hypothesis& hypo,
                             ScoreComponentCollection* accumulator) const
{}

void NieceTerminal::EvaluateChart(const ChartHypothesis &hypo,
                                  ScoreComponentCollection* accumulator) const
{}

bool NieceTerminal::ContainTerm(const InputType &input,
                                const WordsRange &ntRange,
                                const std::set<Word> &terms) const
{
  std::set<Word>::const_iterator iter;

  for (size_t pos = ntRange.GetStartPos(); pos <= ntRange.GetEndPos(); ++pos) {
    const Word &word = input.GetWord(pos);
    iter = terms.find(word);

    if (iter != terms.end()) {
      return true;
    }
  }
  return false;
}

void NieceTerminal::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "hard-constraint") {
    m_hardConstraint = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}


}


