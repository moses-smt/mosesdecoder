// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#ifndef moses_TrellisPathCollection_h
#define moses_TrellisPathCollection_h

#include <set>
#include <iostream>
#include "TrellisPath.h"

namespace Moses
{

struct CompareTrellisPathCollection {
  bool operator()(const TrellisPath* pathA, const TrellisPath* pathB) const {
    return (pathA->GetTotalScore() > pathB->GetTotalScore());
  }
};

/** priority queue used in Manager to store list of contenders for N-Best list.
 * Stored in order of total score so that the best path can just be popped from the top
 *  Used by phrase-based decoding
 */
class TrellisPathCollection
{
  friend std::ostream& operator<<(std::ostream&, const TrellisPathCollection&);

protected:
  typedef std::multiset<TrellisPath*, CompareTrellisPathCollection> CollectionType;
  CollectionType m_collection;

public:
  //iterator begin() { return m_collection.begin(); }
  TrellisPath *pop() {
    TrellisPath *top = *m_collection.begin();

    // Detach
    m_collection.erase(m_collection.begin());
    return top;
  }

  ~TrellisPathCollection() {
    // clean up
    RemoveAllInColl(m_collection);
  }

  //! add a new entry into collection
  void Add(TrellisPath *trellisPath) {
    m_collection.insert(trellisPath);
  }

  size_t GetSize() const {
    return m_collection.size();
  }

  void Prune(size_t newSize);
};

inline std::ostream& operator<<(std::ostream& out, const TrellisPathCollection& pathColl)
{
  TrellisPathCollection::CollectionType::const_iterator iter;

  for (iter = pathColl.m_collection.begin() ; iter != pathColl.m_collection.end() ; ++iter) {
    const TrellisPath &path = **iter;
    out << path << std::endl;
  }
  return out;
}

}

#endif
