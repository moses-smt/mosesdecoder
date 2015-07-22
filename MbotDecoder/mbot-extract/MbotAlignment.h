#pragma once


#include "tables-core.h"

#include <vector>
#include <set>

class MbotAlignment
{
 protected:
  MBOTPHRASE phraseS;
  MBOTPHRASE phraseT;

 public:
  float count;

  std::vector< std::vector< std::set<size_t> > > alignedToS;
  std::vector< std::vector< std::set<size_t> > > alignedToT;

  void create( char*, int, std::string );
  void clear();
  bool equals( const MbotAlignment& );
  bool match( const MbotAlignment& );

  int Compare(const MbotAlignment &compare) const;
  inline bool operator<(const MbotAlignment &compare) const {
    return Compare(compare) < 0;
  }

  const MBOTPHRASE &GetSource() const {
    return phraseS;
  }
  const MBOTPHRASE &GetTarget() const {
    return phraseT;
  }
};

class MbotAlignment;

typedef std::vector<MbotAlignment*> MbotAlignmentCollection;

class MbotAlignmentCollectionOrderer
{
 public:
  bool operator()(const MbotAlignmentCollection &collA, const MbotAlignmentCollection &collB) const {
    assert(collA.size() > 0);
    assert(collB.size() > 0);

    const MbotAlignment &objA = *collA[0];
    const MbotAlignment &objB = *collB[0];
    bool ret = objA < objB;

    return ret;
  }
};

class PhrasePairGroup
{
private:
  typedef std::set<MbotAlignmentCollection, MbotAlignmentCollectionOrderer> Coll;
  Coll m_coll;

public:
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;
  typedef std::vector<const MbotAlignmentCollection *> SortedColl;

  std::pair<Coll::iterator,bool> insert ( const MbotAlignmentCollection& obj );

  const SortedColl &GetSortedColl() const {
    return m_sortedColl;
  }
  size_t GetSize() const {
    return m_coll.size();
  }

private:
  SortedColl m_sortedColl;

};

