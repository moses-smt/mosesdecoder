// $Id$

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
	size_t prevSourcePos = 0;
	AlignmentInfo::const_iterator iter;
	for (iter = alignInfo.begin(); iter != alignInfo.end(); ++iter)
	{
		// alignment pairs must be ordered by source index
		size_t sourcePos = iter->first;
		if (nonTermInd > 0)
		{
			assert(sourcePos > prevSourcePos);
		}
		prevSourcePos = sourcePos;

		size_t targetPos = iter->second;
		m_wordsConsumedTargetOrder[targetPos] = nonTermInd;
		nonTermInd++;
	}
}

std::ostream& operator<<(std::ostream &out, const ChartRule &rule)
{
	out << rule.m_targetPhrase << " " << rule.m_lastWordConsumed;
	return out;
}

} // namespace

