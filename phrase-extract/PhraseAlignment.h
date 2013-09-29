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

namespace MosesTraining
{

// data structure for a single phrase pair
class PhraseAlignment
{
protected:
  PHRASE phraseS;
  PHRASE phraseT;

  void createAlignVec(size_t sourceSize, size_t targetSize);
  void addNTLength(const std::string &tok);
public:
  float pcfgSum;
  float count;
  int sentenceId;
  std::string domain;
  std::string treeFragment;

  std::vector< std::set<size_t> > alignedToT;
  std::vector< std::set<size_t> > alignedToS;

  void create( char*, int, bool );
  void clear();
  bool equals( const PhraseAlignment& );
  bool match( const PhraseAlignment& );

  int Compare(const PhraseAlignment &compare) const;
  inline bool operator<(const PhraseAlignment &compare) const {
    return Compare(compare) < 0;
  }

  const PHRASE &GetSource() const {
    return phraseS;
  }
  const PHRASE &GetTarget() const {
    return phraseT;
  }
};

class PhraseAlignment;

typedef std::vector<PhraseAlignment*>          PhraseAlignmentCollection;
//typedef std::vector<PhraseAlignmentCollection> PhrasePairGroup;

class PhraseAlignmentCollectionOrderer
{
public:
  bool operator()(const PhraseAlignmentCollection &collA, const PhraseAlignmentCollection &collB) const {
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

  const SortedColl &GetSortedColl() const {
    return m_sortedColl;
  }
  size_t GetSize() const {
    return m_coll.size();
  }

private:
  SortedColl m_sortedColl;

};


}

