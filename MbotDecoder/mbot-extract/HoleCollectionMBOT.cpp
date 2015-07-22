// modified for mbot support
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

#include "HoleCollectionMBOT.h"

#include <algorithm>


void HoleCollectionMBOT::SortTargetHoles()
{
  // NIN: assertion fails!!!
  // NIN: need to clear vector when necessary
  //assert(m_sortedTargetHoles.size() == 0);
  if (m_sortedTargetHoles.size() != 0)
    m_sortedTargetHoles.clear();

  // add
  HoleList::iterator iter;
  for (iter = m_holes.begin(); iter != m_holes.end(); ++iter) {
	const HoleMBOT &currHole = *iter;
    std::vector<int> target = currHole.GetStartT();
    int size = target.size();
    if (size > 1) {
      for (std::vector<int>::size_type i=0; i<target.size(); ++i) {
    	m_sortedTargetHoles.push_back(HoleMBOT(currHole, i+1));
      }
    } else {
    	m_sortedTargetHoles.push_back(*iter);
    }
  }

  // sort
  std::sort(m_sortedTargetHoles.begin(), m_sortedTargetHoles.end(), HoleTargetOrderer());
  }

void HoleCollectionMBOT::Add(int startS, int endS, std::vector<std::pair<int,int> > holeSpanT)
{
  HoleMBOT hole(startS, endS, holeSpanT);
  m_scope = Scope(hole);
  m_sourceHoleStartPoints.insert(startS);
  m_sourceHoleEndPoints.insert(endS);
  m_holes.push_back(hole);
}

int HoleCollectionMBOT::Scope(const HoleMBOT &proposedHole) const
{
  const int holeStart = proposedHole.GetStartS();
  const int holeEnd = proposedHole.GetEndS();
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

