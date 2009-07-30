/*
 *  Word.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <vector>

class Word
{
	std::vector<std::string> m_factors;
public:
	void CreateFromString(const std::string &inString);
};

