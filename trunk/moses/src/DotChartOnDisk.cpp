// $Id: DotChartOnDisk.cpp 3048 2010-04-05 17:25:26Z hieuhoang1972 $
#include <algorithm>
#include "DotChartOnDisk.h"
#include "Util.h"
#include "PhraseDictionaryNodeNewFormat.h"
#include "../../OnDiskPt/src/PhraseNode.h"

using namespace std;

namespace Moses
{
ProcessedRuleStackOnDisk::ProcessedRuleStackOnDisk(size_t size)
:m_coll(size)
{
	for (size_t ind = 0; ind < size; ++ind)
	{
		m_coll[ind] = new ProcessedRuleCollOnDisk();
	}
}

ProcessedRuleStackOnDisk::~ProcessedRuleStackOnDisk()
{
	RemoveAllInColl(m_coll);
	RemoveAllInColl(m_savedNode);
}

class SavedNodesOderer
{
public:
	bool operator()(const SavedNodeOnDisk* a, const SavedNodeOnDisk* b) const
	{
		bool ret = a->GetProcessedRule().GetLastNode().GetCount(0) > b->GetProcessedRule().GetLastNode().GetCount(0);
		return ret;
	}
};
	
void ProcessedRuleStackOnDisk::SortSavedNodes()
{
	sort(m_savedNode.begin(), m_savedNode.end(), SavedNodesOderer());
}
	
std::ostream& operator<<(std::ostream &out, const ProcessedRuleOnDisk &rule)
{
  //const MosesBerkeleyPt::SourcePhraseNode &node = rule.GetLastNode();
  //out << node;
	
	return out;
}

std::ostream& operator<<(std::ostream &out, const ProcessedRuleCollOnDisk &coll)
{
	ProcessedRuleCollOnDisk::CollType::const_iterator iter;
	for (iter = coll.begin(); iter != coll.end(); ++iter)
	{
		const ProcessedRuleOnDisk &rule = **iter;
		out << rule << endl;
		
	}
	
	return out;
}

};
