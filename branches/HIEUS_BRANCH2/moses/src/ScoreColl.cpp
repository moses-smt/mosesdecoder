
#include "ScoreColl.h"


void ScoreColl::Combine(const ScoreColl &other)
{
	const_iterator iter;
	for (iter = other.begin() ; iter != other.end() ; iter++)
	{
		float value = iter->second;
		iterator iterThis = find(iter->first);
		assert (iterThis != end());
		
		// score component for dictionary exists. add numbers
		iterThis->second += value;
	}
}
