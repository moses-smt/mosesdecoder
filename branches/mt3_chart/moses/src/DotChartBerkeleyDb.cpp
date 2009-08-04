#include "DotChartBerkeleyDb.h"
#include "Util.h"
#include "PhraseDictionaryNodeSourceLabel.h"
#include "../../BerkeleyPt/src/SourcePhraseNode.h"

using namespace std;

namespace Moses
{
ProcessedRuleStackBerkeleyDb::ProcessedRuleStackBerkeleyDb(size_t size)
:m_coll(size)
{
	for (size_t ind = 0; ind < size; ++ind)
	{
		m_coll[ind] = new ProcessedRuleCollBerkeleyDb();
	}
}

ProcessedRuleStackBerkeleyDb::~ProcessedRuleStackBerkeleyDb()
{
	RemoveAllInColl(m_coll);
	RemoveAllInColl(m_savedNode);
}

std::ostream& operator<<(std::ostream &out, const ProcessedRuleBerkeleyDb &rule)
{
	const MosesBerkeleyPt::SourcePhraseNode &node = rule.GetLastNode();
	out << node;
	
	return out;
}

std::ostream& operator<<(std::ostream &out, const ProcessedRuleCollBerkeleyDb &coll)
{
	ProcessedRuleCollBerkeleyDb::CollType::const_iterator iter;
	for (iter = coll.begin(); iter != coll.end(); ++iter)
	{
		const ProcessedRuleBerkeleyDb &rule = **iter;
		out << rule << endl;
		
	}
	
	return out;
}

};
