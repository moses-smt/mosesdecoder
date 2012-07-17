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

namespace MosesTraining
{

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

class LexicalTable
{
public:
  std::map< WORD_ID, std::map< WORD_ID, double > > ltable;
  void load( char[] );
  double permissiveLookup( WORD_ID wordS, WORD_ID wordT ) {
    // cout << endl << vcbS.getWord( wordS ) << "-" << vcbT.getWord( wordT ) << ":";
    if (ltable.find( wordS ) == ltable.end()) return 1.0;
    if (ltable[ wordS ].find( wordT ) == ltable[ wordS ].end()) return 1.0;
    // cout << ltable[ wordS ][ wordT ];
    return ltable[ wordS ][ wordT ];
  }
};

// other functions *********************************************
inline bool isNonTerminal( const std::string &word )
{
  return (word.length()>=3 && word[0] == '[' && word[word.length()-1] == ']');
}

  
}

