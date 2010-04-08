
#include "ChartTranslationOption.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/ChartRule.h"
#include "../../moses/src/WordConsumed.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

TranslationOption::TranslationOption(const WordsRange &wordsRange
								, const ChartRule &rule)
:m_rule(rule)
,m_wordsRange(wordsRange)
{
	//assert(wordsRange.GetStartPos() == rule.GetWordsConsumed().front()->GetWordsRange().GetStartPos());
	assert(wordsRange.GetEndPos() == rule.GetLastWordConsumed().GetWordsRange().GetEndPos());
}

TranslationOption::~TranslationOption()
{

}

// friend
ostream& operator<<(ostream& out, const TranslationOption& transOpt)
{
	out << transOpt.GetTotalScore()
			<< " " << transOpt.GetChartRule().GetTargetPhrase()
			<< " " << transOpt.GetChartRule().GetTargetPhrase().GetScoreBreakdown()
			<< " " << transOpt.m_rule;
	
	return out;
}

}

