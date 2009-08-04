
#include <iostream>
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

}

