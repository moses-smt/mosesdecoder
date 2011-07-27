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
#include <vector>

class PhraseAlignment;

typedef std::vector<PhraseAlignment*>          PhraseAlignmentCollection;
//typedef std::vector<PhraseAlignmentCollection> PhrasePairGroup;

class PhraseAlignmentCollectionOrderer
{
public:
	bool operator()(const PhraseAlignmentCollection &collA, const PhraseAlignmentCollection &collB) const
	{
    assert(collA.size() > 0);
    assert(collB.size() > 0);

    const PhraseAlignment &objA = *collA[0];
    const PhraseAlignment &objB = *collB[0];
    bool ret = objA < objB;

    return ret;
	}
};


typedef std::set<PhraseAlignmentCollection, PhraseAlignmentCollectionOrderer> PhrasePairGroup;

inline bool isNonTerminal( std::string &word )
{
  return (word.length()>=3 &&
          word.substr(0,1).compare("[") == 0 &&
          word.substr(word.length()-1,1).compare("]") == 0);
}


