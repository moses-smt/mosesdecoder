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
#include <map>

// data structure for a single phrase pair
class PhraseAlignment
{
protected:
  PHRASE phraseS;
  PHRASE phraseT;

  std::map<size_t, std::pair<size_t, size_t> > m_ntLengths;
  
  void createAlignVec(size_t sourceSize, size_t targetSize);
  void addNTLength(const std::string &tok);
public:
  float pcfgSum;
  float count;
  std::vector< std::set<size_t> > alignedToT;
  std::vector< std::set<size_t> > alignedToS;

  void create( char*, int );
  void clear();
  bool equals( const PhraseAlignment& );
  bool match( const PhraseAlignment& );

	int Compare(const PhraseAlignment &compare) const;
	inline bool operator<(const PhraseAlignment &compare) const
	{ 
		return Compare(compare) < 0;
	}

  const PHRASE &GetSource() const {
    return phraseS;
  }
  const PHRASE &GetTarget() const {
    return phraseT;
  }
  
  const std::map<size_t, std::pair<size_t, size_t> > &GetNTLengths() const
  { return m_ntLengths; }

};
