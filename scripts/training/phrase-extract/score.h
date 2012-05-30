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


//typedef std::set<PhraseAlignmentCollection, PhraseAlignmentCollectionOrderer> PhrasePairGroup;

class PhrasePairGroup
{
private:
  typedef std::set<PhraseAlignmentCollection, PhraseAlignmentCollectionOrderer> Coll;
  Coll m_coll;


public:
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;
  typedef std::vector<const PhraseAlignmentCollection *> SortedColl;

  std::pair<Coll::iterator,bool> insert ( const PhraseAlignmentCollection& obj );

  const SortedColl &GetSortedColl() const
  { return m_sortedColl; }
  size_t GetSize() const
  { return m_coll.size(); }

private:
  SortedColl m_sortedColl;

};

// other functions *********************************************
inline bool isNonTerminal( const std::string &word )
{
  return (word.length()>=3 && word[0] == '[' && word[word.length()-1] == ']');
}
