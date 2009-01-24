
#include "ChartTranslationOption.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/ChartRule.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

TranslationOption::TranslationOption(const WordsRange &wordsRange
								, const ChartRule &rule
								, const InputType &inputType)
:m_rule(rule)
,m_wordsRange(wordsRange)
{
	assert(wordsRange.GetStartPos() == rule.GetWordsConsumed().front().GetWordsRange().GetStartPos());
	assert(wordsRange.GetEndPos() == rule.GetWordsConsumed().back().GetWordsRange().GetEndPos());
}

TranslationOption::~TranslationOption()
{

}


// friend
ostream& operator<<(ostream& out, const TranslationOption& transOpt)
{
	out << transOpt.GetChartRule().GetTargetPhrase();
	return out;
}

}

