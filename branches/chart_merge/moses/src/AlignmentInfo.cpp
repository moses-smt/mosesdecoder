#include "AlignmentInfo.h"

namespace Moses
{

std::ostream& operator<<(std::ostream &out, const AlignmentInfo &alignmentInfo)
{
	AlignmentInfo::const_iterator iter;
	for (iter = alignmentInfo.begin(); iter != alignmentInfo.end(); ++iter)
	{
		out << "(" << iter->first << "," << iter->second << ") ";
	}
	return out;
}

void AlignmentInfo::AddAlignment(const std::list<std::pair<size_t,size_t> > &alignmentPairs)
{
	m_collection = alignmentPairs;
	m_collection.sort();
}

}

