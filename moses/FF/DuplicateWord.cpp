#include <vector>
#include <set>
#include "DuplicateWord.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartCellLabel.h"

using namespace std;

namespace Moses
{
void DuplicateWord::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void DuplicateWord::Evaluate(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
  const Phrase *ruleSource = targetPhrase.GetRuleSource();
  assert(ruleSource);

  std::set<Word> terms;
  for (size_t i = 0; i < ruleSource->GetSize(); ++i) {
	  const Word &word = ruleSource->GetWord(i);
	  if (word.IsNonTerminal()) {
		  terms.insert(word);
	  }
  }

  size_t ntInd = 0;
  for (size_t i = 0; i < ruleSource->GetSize(); ++i) {
	  const Word &word = ruleSource->GetWord(i);
	  if (!word.IsNonTerminal()) {
		  const ChartCellLabel &cell = *stackVec->at(ntInd);
		  const WordsRange &ntRange = cell.GetCoverage();
		  bool containTerm = ContainTerm(ntRange, terms);

		  if (containTerm) {
			  scoreBreakdown.PlusEquals(this, 1);
			  return;
		  }
		  ++ntInd;
	  }
  }

}

void DuplicateWord::Evaluate(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void DuplicateWord::EvaluateChart(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

bool DuplicateWord::ContainTerm(const WordsRange &ntRange, const std::set<Word> &terms) const
{
	std::set<Word>::const_iterator iter;

	for (size_t pos = ntRange.GetStartPos(); pos <= ntRange.GetEndPos(); ++pos) {

//		iter = terms.find(ntRange);
	}
	return false;
}

}

