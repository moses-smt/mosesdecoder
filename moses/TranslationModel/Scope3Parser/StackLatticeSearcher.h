/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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

#include "StackLattice.h"
#include "VarSpanNode.h"
#include "moses/StackVec.h"

#include <vector>

namespace Moses
{

class ChartHypothesisCollection;

template<typename MatchCallBackType>
class StackLatticeSearcher
{
public:
  StackLatticeSearcher(const StackLattice &lattice,
                       const std::vector<VarSpanNode::NonTermRange> &ranges)
    : m_lattice(lattice)
    , m_ranges(ranges) {}

  void Search(const std::vector<int> &labels, MatchCallBackType &callback) {
    m_labels = &labels;
    m_matchCB = &callback;
    SearchInner(0, 0);
  }

private:
  void SearchInner(int start, size_t index) {
    assert(m_stackVec.size() == index);

    const VarSpanNode::NonTermRange &range = m_ranges[index];

    const size_t offset = (range.s1 == range.s2) ? range.s1 : start;

    const size_t minSpan = std::max(offset, range.e1) - offset + 1;
    const size_t maxSpan = range.e2 - offset + 1;

    // Loop over all possible spans for this offset and index.
    const std::vector<StackVec> &spanVec = m_lattice[offset][index];

    for (size_t j = minSpan; j <= maxSpan; ++j) {
      const ChartCellLabel *stack = spanVec[j][(*m_labels)[index]];
      if (!stack) {
        continue;
      }
      m_stackVec.push_back(stack);
      if (index+1 == m_labels->size()) {
        (*m_matchCB)(m_stackVec);
      } else {
        SearchInner(offset+j, index+1);
      }
      m_stackVec.pop_back();
    }
  }

  const StackLattice &m_lattice;
  const std::vector<VarSpanNode::NonTermRange> &m_ranges;
  const std::vector<int> *m_labels;
  MatchCallBackType *m_matchCB;
  StackVec m_stackVec;
};

}  // namespace Moses
