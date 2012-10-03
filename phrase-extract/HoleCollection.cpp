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

#include "HoleCollection.h"

#include <algorithm>

namespace MosesTraining
{

void HoleCollection::SortSourceHoles()
{
  assert(m_sortedSourceHoles.size() == 0);

  // add
  HoleList::iterator iter;
  for (iter = m_holes.begin(); iter != m_holes.end(); ++iter) {
    Hole &currHole = *iter;
    m_sortedSourceHoles.push_back(&currHole);
  }

  // sort
  std::sort(m_sortedSourceHoles.begin(), m_sortedSourceHoles.end(), HoleSourceOrderer());
}

void HoleCollection::Add(int startT, int endT, int startS, int endS)
{
  Hole hole(startS, endS, startT, endT);
  m_scope = Scope(hole);
  m_sourceHoleStartPoints.insert(startS);
  m_sourceHoleEndPoints.insert(endS);
  m_holes.push_back(hole);
}

int HoleCollection::Scope(const Hole &proposedHole) const
{
  const int holeStart = proposedHole.GetStart(0);
  const int holeEnd = proposedHole.GetEnd(0);
  int scope = m_scope;
  if (holeStart == m_sourcePhraseStart ||
      m_sourceHoleEndPoints.find(holeStart-1) != m_sourceHoleEndPoints.end()) {
    ++scope; // Adding hole would introduce choice point at start of hole.
  }
  if (holeEnd == m_sourcePhraseEnd ||
      m_sourceHoleStartPoints.find(holeEnd+1) != m_sourceHoleStartPoints.end()) {
    ++scope; // Adding hole would introduce choice point at end of hole.
  }
  return scope;
}

}
