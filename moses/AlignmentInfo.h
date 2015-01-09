/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <iostream>
#include <ostream>
#include <set>
#include <vector>
#include <cstdlib>

#include <boost/functional/hash.hpp>

namespace Moses
{

class AlignmentInfoCollection;

/** Collection of non-terminal alignment pairs, ordered by source index.
  * Usually held by a TargetPhrase to map non-terms in hierarchical/syntax models
 */
class AlignmentInfo
{
  friend std::ostream& operator<<(std::ostream &, const AlignmentInfo &);
  friend struct AlignmentInfoOrderer;
  friend struct AlignmentInfoHasher;
  friend class AlignmentInfoCollection;
  friend class VW;

public:
  typedef std::set<std::pair<size_t,size_t> > CollType;
  typedef std::vector<size_t> NonTermIndexMap;
  typedef CollType::const_iterator const_iterator;

  const_iterator begin() const {
    return m_collection.begin();
  }
  const_iterator end() const {
    return m_collection.end();
  }

  void Add(size_t sourcePos, size_t targetPos) {
    m_collection.insert(std::pair<size_t, size_t>(sourcePos, targetPos));
  }
  /** Provides a map from target-side to source-side non-terminal indices.
    * The target-side index should be the rule symbol index (COUNTING terminals).
    * The index returned is the rule non-terminal index (IGNORING terminals).
   */
  const NonTermIndexMap &GetNonTermIndexMap() const {
    return m_nonTermIndexMap;
  }

  /** Like GetNonTermIndexMap but the return value is the symbol index (i.e.
    * the index counting both terminals and non-terminals) */
  const NonTermIndexMap &GetNonTermIndexMap2() const {
    return m_nonTermIndexMap2;
  }

  const CollType &GetAlignments() const {
    return m_collection;
  }

  std::set<size_t> GetAlignmentsForSource(size_t sourcePos) const;
  std::set<size_t> GetAlignmentsForTarget(size_t targetPos) const;

  size_t GetSize() const {
    return m_collection.size();
  }

  std::vector< const std::pair<size_t,size_t>* > GetSortedAlignments() const;

  std::vector<size_t> GetSourceIndex2PosMap() const;

  bool operator==(const AlignmentInfo& rhs) const {
    return m_collection == rhs.m_collection &&
           m_nonTermIndexMap == rhs.m_nonTermIndexMap;
  }

private:
  //! AlignmentInfo objects should only be created by an AlignmentInfoCollection
  explicit AlignmentInfo(const std::set<std::pair<size_t,size_t> > &pairs);
  explicit AlignmentInfo(const std::vector<unsigned char> &aln);

  // used only by VW to load word alignment between sentences
  explicit AlignmentInfo(const std::string &str);

  void BuildNonTermIndexMaps();

  CollType m_collection;
  NonTermIndexMap m_nonTermIndexMap;
  NonTermIndexMap m_nonTermIndexMap2;
};

/** Define an arbitrary strict weak ordering between AlignmentInfo objects
 * for use by AlignmentInfoCollection.
 */
struct AlignmentInfoOrderer {
  bool operator()(const AlignmentInfo &a, const AlignmentInfo &b) const {
    if (a.m_collection == b.m_collection) {
      return a.m_nonTermIndexMap < b.m_nonTermIndexMap;
    } else {
      return a.m_collection < b.m_collection;
    }
  }
};

/**
 * Hashing functoid
 **/
struct AlignmentInfoHasher {
  size_t operator()(const AlignmentInfo& a) const {
    size_t seed = 0;
    boost::hash_combine(seed,a.m_collection);
    boost::hash_combine(seed,a.m_nonTermIndexMap);
    return seed;
  }

};

inline size_t hash_value(const AlignmentInfo& a)
{
  static AlignmentInfoHasher hasher;
  return hasher(a);
}

}
