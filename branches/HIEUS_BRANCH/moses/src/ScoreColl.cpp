
#include "ScoreColl.h"

ScoreColl::ScoreColl(const ScoreColl &copy)
{
	const_iterator iter;
	for (iter = copy.begin() ; iter != copy.end() ; iter++)
	{
		// score component for dictionary exists. add numbers
		Add(iter->first);
		SetValue(iter->first, iter->second);
	}
}

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
