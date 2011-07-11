/*
 *  RuleExist.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "RuleExist.h"

using namespace std;

std::ostream& operator<<(std::ostream &out, const RuleExist &ruleExist)
{
	size_t size = ruleExist.GetSize();
	
	for (size_t startPos = 0; startPos < size; ++startPos)
	{
		for (size_t endPos = startPos; endPos < size; ++endPos)
		{
			const HoleList &holeList = ruleExist.GetSourceHoles(startPos, endPos);
			HoleList::const_iterator iter;
			for (iter = holeList.begin(); iter != holeList.end(); ++iter)
			{
				const Hole &hole = *iter;
				out << hole << " ";
				
			}
 		}
	}
	
	return out;
}


