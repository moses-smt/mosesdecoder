
#include "ChartRule.h"
#include "TargetPhrase.h"
#include "AlignmentInfo.h"
#include "WordConsumed.h"

using namespace std;

namespace Moses
{

void ChartRule::CreateNonTermIndex()
{
	m_wordsConsumedTargetOrder.resize(m_targetPhrase.GetSize(), NOT_FOUND);
	const AlignmentInfo &alignInfo = m_targetPhrase.GetAlignmentInfo();

	size_t nonTermInd = 0;
	AlignmentInfo::const_iterator iter;
	for (iter = alignInfo.begin(); iter != alignInfo.end(); ++iter)
	{
		// just for assert
		//size_t sourcePos = iter->first;

		size_t targetPos = iter->second;
		m_wordsConsumedTargetOrder[targetPos] = nonTermInd;
		nonTermInd++;
	}
}

bool ChartRule::operator<(const ChartRule &compare) const
{
	bool ret;
	
	int cmp = m_lastWordConsumed.CompareWordsRange(compare.m_lastWordConsumed);
	if (cmp == 0)
	{
		ret = m_targetPhrase < compare.m_targetPhrase;
	}
	else
	{
		ret = cmp < 0;		
	}
	
	return ret;
}

std::ostream& operator<<(std::ostream &out, const ChartRule &rule)
{
	out << rule.m_lastWordConsumed << " " << rule.m_targetPhrase ;
	return out;
}

} // namespace

