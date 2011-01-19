#pragma once
/*
 *  PhraseAlignment.h
 *  extract
 *
 *  Created by Hieu Hoang on 28/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include "tables-core.h"

#include <vector>
#include <set>

// data structure for a single phrase pair
class PhraseAlignment 
{
protected:
  PHRASE phraseS;
  PHRASE phraseT;

	void createAlignVec(size_t sourceSize, size_t targetSize);
public:
	float count;
	std::vector< std::set<size_t> > alignedToT;
	std::vector< std::set<size_t> > alignedToS;
	
	void create( char*, int );
	void clear();
	bool equals( const PhraseAlignment& );
	bool match( const PhraseAlignment& );
	
  const PHRASE &GetSource() const { return phraseS; }
  const PHRASE &GetTarget() const { return phraseT; }
};
