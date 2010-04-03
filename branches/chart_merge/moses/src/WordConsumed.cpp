
#include "WordConsumed.h"

namespace Moses
{

std::ostream& operator<<(std::ostream &out, const WordConsumed &wordConsumed)
{
	out << wordConsumed.m_coverage;
	if (wordConsumed.m_prevWordsConsumed)
		out << " " << *wordConsumed.m_prevWordsConsumed;
	
	return out;
}


} // namespace
