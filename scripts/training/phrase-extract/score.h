#pragma once
/*
 *  score.h
 *  extract
 *
 *  Created by Hieu Hoang on 28/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>


inline bool isNonTerminal( std::string &word ) 
{
	return (word.length()>=3 &&
					word.substr(0,1).compare("[") == 0 && 
					word.substr(word.length()-1,1).compare("]") == 0);
}
