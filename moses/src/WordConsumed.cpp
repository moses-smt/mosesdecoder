
#include "WordConsumed.h"

namespace Moses
{

int WordConsumed::CompareWordsRange(const WordConsumed &compare) const
{
	int ret;

	if (m_coverage == compare.m_coverage)
	{
		if (m_prevWordsConsumed && compare.m_prevWordsConsumed == NULL)
			ret = 1;
		else if (m_prevWordsConsumed == NULL && compare.m_prevWordsConsumed)
			ret = -1;
		else if (m_prevWordsConsumed == NULL && compare.m_prevWordsConsumed == NULL)
			ret = 0;
		else
			ret = m_prevWordsConsumed->CompareWordsRange(*compare.m_prevWordsConsumed);
	}
	else
	{
		if (m_coverage == compare.m_coverage)
			ret = 0;
		else if (m_coverage < compare.m_coverage)
			ret = -1;
		else
			ret = 1;
	}
	
	return ret;
}

std::ostream& operator<<(std::ostream &out, const WordConsumed &wordConsumed)
{
	//recursive, backward
	const WordConsumed *prev = wordConsumed.GetPrevWordsConsumed();
	if (prev)
		out << *prev << " ";

	const Word *otherWord = wordConsumed.GetOtherWord();
	out << wordConsumed.GetWordsRange() << "=";
	if (otherWord)
		out << *otherWord;
	out << wordConsumed.GetMainWord();
	
	return out;
}


} // namespace
