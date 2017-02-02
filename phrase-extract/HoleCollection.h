/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

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
#ifndef HOLECOLLECTION_H_INCLUDED_
#define HOLECOLLECTION_H_INCLUDED_

#include <set>
#include <vector>

#include "Hole.h"

namespace MosesTraining
{

class HoleCollection
{
protected:
  HoleList m_holes;
  std::vector<Hole*> m_sortedSourceHoles;
  std::vector<int> m_sourceHoleStartPoints;
  std::vector<int> m_sourceHoleEndPoints;
  std::vector<int> m_scope;
  std::vector<int> m_sourcePhraseStart;
  std::vector<int> m_sourcePhraseEnd;

public:
  HoleCollection(int sourcePhraseStart, int sourcePhraseEnd)
    : m_scope(1, 0)
    , m_sourcePhraseStart(1, sourcePhraseStart)
    , m_sourcePhraseEnd(1, sourcePhraseEnd) {
  }

  const HoleList &GetHoles() const {
    return m_holes;
  }

  HoleList &GetHoles() {
    return m_holes;
  }

  std::vector<Hole*> &GetSortedSourceHoles() {
    return m_sortedSourceHoles;
  }

  void Add(int startT, int endT, int startS, int endS);

  void RemoveLast();

  bool OverlapSource(const Hole &sourceHole) const {
    HoleList::const_iterator iter;
    for (iter = m_holes.begin(); iter != m_holes.end(); ++iter) {
      const Hole &currHole = *iter;
      if (currHole.Overlap(sourceHole, 0))
        return true;
    }
    return false;
  }

  bool ConsecSource(const Hole &sourceHole) const {
    HoleList::const_iterator iter;
    for (iter = m_holes.begin(); iter != m_holes.end(); ++iter) {
      const Hole &currHole = *iter;
      if (currHole.Neighbor(sourceHole, 0))
        return true;
    }
    return false;
  }

  // Determine the scope that would result from adding the given hole.
  int Scope(const Hole &proposedHole) const;

  void SortSourceHoles();

};

}

#endif
