
#include "ChartRule.h"
#include "TargetPhrase.h"
#include "AlignmentInfo.h"

using namespace std;

namespace Moses
{

ChartRule::ChartRule(const TargetPhrase &targetPhrase, const std::vector<WordsConsumed> &wordsConsumed)
:m_targetPhrase(targetPhrase)
,m_wordsConsumed(wordsConsumed)
,m_wordsConsumedTargetOrder(targetPhrase.GetSize(), NOT_FOUND)
{
	const AlignmentInfo &alignInfo = m_targetPhrase.GetAlignmentInfo();

	size_t nonTermInd = 0;
	AlignmentInfo::const_iterator iter;
	for (iter = alignInfo.begin(); iter != alignInfo.end(); ++iter)
	{
		// just for assert
		size_t sourcePos = iter->first;
		const WordsConsumed &wordsConsumed = m_wordsConsumed[sourcePos];
		assert(wordsConsumed.IsNonTerminal());

		size_t targetPos = iter->second;
		m_wordsConsumedTargetOrder[targetPos] = nonTermInd;
		nonTermInd++;
	}
}

} // namespace

