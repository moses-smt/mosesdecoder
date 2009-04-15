#include "DotChart.h"
#include "Util.h"

namespace Moses
{
ProcessedRuleStack::ProcessedRuleStack(size_t size)
:m_coll(size)
{
	for (size_t ind = 0; ind < size; ++ind)
	{
		m_coll[ind] = new ProcessedRuleColl();
	}
}

ProcessedRuleStack::~ProcessedRuleStack()
{
	RemoveAllInColl(m_coll);
	RemoveAllInColl(m_savedNode);
}

};
