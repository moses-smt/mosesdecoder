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

#include <ostream>
#include <set>
#include <vector>

namespace Moses
{

class AlignmentInfoCollection;

/** Collection of non-terminal alignment pairs, ordered by source index.
  * Usually held by a TargetPhrase to map non-terms in hierarchical/syntax models
 */
class AlignmentInfo
{
  typedef std::set<std::pair<size_t,size_t> > CollType;

  friend std::ostream& operator<<(std::ostream &, const AlignmentInfo &);
  friend struct AlignmentInfoOrderer;
  friend class AlignmentInfoCollection;

 public:
  typedef std::vector<size_t> NonTermIndexMap;
  typedef CollType::const_iterator const_iterator;

  const_iterator begin() const { return m_collection.begin(); }
  const_iterator end() const { return m_collection.end(); }

  // Provides a map from target-side to source-side non-terminal indices.
  // The target-side index should be the rule symbol index (counting terminals).
  // The index returned is the rule non-terminal index (ignoring terminals).
  const NonTermIndexMap &GetNonTermIndexMap() const {
    return m_nonTermIndexMap;
  }

  size_t GetSize() const { return m_collection.size(); }

  std::vector< const std::pair<size_t,size_t>* > GetSortedAlignments() const;
  
 private:
  // AlignmentInfo objects should only be created by an AlignmentInfoCollection
  explicit AlignmentInfo(const std::set<std::pair<size_t,size_t> > &pairs)
    : m_collection(pairs)
  {
    BuildNonTermIndexMap();
  }

  void BuildNonTermIndexMap();

  CollType m_collection;
  NonTermIndexMap m_nonTermIndexMap;
};

/** Define an arbitrary strict weak ordering between AlignmentInfo objects
 * for use by AlignmentInfoCollection.
 */
struct AlignmentInfoOrderer
{
  bool operator()(const AlignmentInfo &a, const AlignmentInfo &b) const {
    return a.m_collection < b.m_collection;
  }
};

}
