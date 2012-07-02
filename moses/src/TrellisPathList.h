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

#ifndef moses_TrellisPathList_h
#define moses_TrellisPathList_h

#include <list>
#include <set>
#include "TrellisPath.h"

namespace Moses
{

/** used to return n-best list of Trellis Paths from the Manager to the caller.
 *  Used by phrase-based decoding
 */
class TrellisPathList
{
protected:
  std::list<const TrellisPath*> m_collection;
public:
  // iters
  typedef std::list<const TrellisPath*>::iterator iterator;
  typedef std::list<const TrellisPath*>::const_iterator const_iterator;

  iterator begin() {
    return m_collection.begin();
  }
  iterator end() {
    return m_collection.end();
  }
  const_iterator begin() const {
    return m_collection.begin();
  }
  const_iterator end() const {
    return m_collection.end();
  }

  ~TrellisPathList() {
    // clean up
    RemoveAllInColl(m_collection);
  }

  //! add a new entry into collection
  void Add(TrellisPath *trellisPath) {
    m_collection.push_back(trellisPath);
  }

  const TrellisPath *pop() {
    const TrellisPath *top = m_collection.front();

    // Detach
    m_collection.pop_front();
    return top;
  }

  size_t GetSize() const {
    return m_collection.size();
  }

  const TrellisPath at(size_t position) const {
    const_iterator iter = m_collection.begin();
    for(size_t i = position; i>0; i--) {
      iter++;
    }
    return **iter;
  }
};

}

#endif
