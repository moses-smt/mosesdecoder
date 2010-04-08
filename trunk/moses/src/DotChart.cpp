// $Id: DotChart.cpp 3048 2010-04-05 17:25:26Z hieuhoang1972 $
#include "DotChart.h"
#include "Util.h"
#include "PhraseDictionaryNodeNewFormat.h"

using namespace std;

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

std::ostream& operator<<(std::ostream &out, const ProcessedRule &rule)
{
	const PhraseDictionaryNode &node = rule.GetLastNode();
	//out << node;
	
	return out;
}

std::ostream& operator<<(std::ostream &out, const ProcessedRuleColl &coll)
{
	ProcessedRuleColl::CollType::const_iterator iter;
	for (iter = coll.begin(); iter != coll.end(); ++iter)
	{
		const ProcessedRule &rule = **iter;
		out << rule << endl;
		
	}
	
	return out;
}

};
