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
  const Phrase *ruleSource = targetPhrase.GetRuleSource();
  assert(ruleSource);

  std::set<Word> terms;
  for (size_t i = 0; i < ruleSource->GetSize(); ++i) {
	  const Word &word = ruleSource->GetWord(i);
	  if (!word.IsNonTerminal()) {
		  terms.insert(word);
	  }
  }

  size_t ntInd = 0;
  for (size_t i = 0; i < ruleSource->GetSize(); ++i) {
	  const Word &word = ruleSource->GetWord(i);
	  if (word.IsNonTerminal()) {
		  const ChartCellLabel &cell = *stackVec->at(ntInd);
		  const WordsRange &ntRange = cell.GetCoverage();
		  bool containTerm = ContainTerm(input, ntRange, terms);

		  if (containTerm) {
			  //cerr << "ruleSource=" << *ruleSource << " ";
			  //cerr << "ntRange=" << ntRange << endl;

			  // non-term contains 1 of the terms in the rule.
			  scoreBreakdown.PlusEquals(this, 1);
			  return;
		  }
		  ++ntInd;
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

}

