/*
 *  Word.cpp
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Word.h"
#include "../../moses/src/Util.h"

void Word::CreateFromString(const std::string &inString)
{
	m_factors = Moses::Tokenize(inString, "|");
}

