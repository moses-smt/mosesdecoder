
#include "ChartRule.h"
#include "TargetPhrase.h"

using namespace std;

namespace Moses
{

ChartRule::ChartRule(const TargetPhrase &targetPhrase, const std::vector<WordsConsumed> &wordsConsumed)
:m_targetPhrase(targetPhrase)
,m_wordsConsumed(wordsConsumed)
,m_wordsConsumedTargetOrder(targetPhrase.GetSize(), NOT_FOUND)
{
	/* TODO
	const AlignmentPhrase &sourceAlign = m_targetPhrase.GetAlignmentPair().GetAlignmentPhrase(Input);
	assert(m_wordsConsumed.size() == sourceAlign.GetSize());

	size_t nonTermInd = 0;
	for (size_t sourcePos = 0; sourcePos < m_wordsConsumed.size(); ++sourcePos)
	{
		const WordsConsumed &wordsConsumed = m_wordsConsumed[sourcePos];
		if (wordsConsumed.IsNonTerminal())
		{
			const AlignmentElement &alignElement = sourceAlign.GetElement(sourcePos);
			assert(alignElement.GetSize() == 1);

			size_t targetPos = *alignElement.GetCollection().begin();
			m_wordsConsumedTargetOrder[targetPos] = nonTermInd;
			nonTermInd++;
		}
	}
	*/
}

}

