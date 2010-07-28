#pragma once
/*
 *  PhraseAlignment.h
 *  extract
 *
 *  Created by Hieu Hoang on 28/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include <set>

// data structure for a single phrase pair
class PhraseAlignment 
{
protected:
	int target, source;

public:
	float count;
	std::vector< std::set<size_t> > alignedToT;
	std::vector< std::set<size_t> > alignedToS;
  
	void create( char*, int );
	void addToCount( char* );
	void clear();
	bool equals( const PhraseAlignment& );
	bool match( const PhraseAlignment& );
	
	int GetTarget() const
	{ return target; }
	int GetSource() const
	{ return source; }
	
};
