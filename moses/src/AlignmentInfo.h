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
#include <map>
#include <vector>

namespace Moses
{

class AlignmentInfoCollection;

/** Collection of non-terminal alignment pairs, ordered by source index.
  * Usually held by a TargetPhrase to map non-terms in hierarchical/syntax models
 */
class AlignmentInfo
{
  typedef std::multimap<size_t,size_t> CollType;

  friend std::ostream& operator<<(std::ostream &, const AlignmentInfo &);
  friend struct AlignmentInfoOrderer;
  friend class AlignmentInfoCollection;

 public:
  typedef CollType::const_iterator const_iterator;

  const_iterator begin() const { return m_collection.begin(); }
  const_iterator end() const { return m_collection.end(); }

  size_t GetSize() const { return m_collection.size(); }

  std::vector< const std::pair<size_t,size_t>* > GetSortedAlignments() const;
  
 private:
  //! AlignmentInfo objects should only be created by an AlignmentInfoCollection
  explicit AlignmentInfo(const std::set<std::pair<size_t,size_t> > &pairs)
    : m_collection(pairs.begin(), pairs.end())
  {
  }


  CollType m_collection;
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

class StringPiece;
namespace Moses {
  std::set< std::pair<size_t, size_t> > ParseAlignmentFromString(const StringPiece &str);
}
