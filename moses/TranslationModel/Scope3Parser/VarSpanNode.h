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

#include "IntermediateVarSpanNode.h"
#include "moses/Range.h"

#include <boost/array.hpp>

#include <map>

#include <vector>

namespace Moses
{

/** @todo what is this?
 */
struct VarSpanNode {
public:
  struct NonTermRange {
    size_t s1;
    size_t s2;
    size_t e1;
    size_t e2;
  };
  typedef std::vector<IntermediateVarSpanNode> NodeVec;
  typedef boost::array<int, 5> KeyType;
  typedef std::map<KeyType, VarSpanNode> MapType;

  VarSpanNode() : m_parent(0), m_label(0), m_rank(0) {}

  VarSpanNode &Insert(const NodeVec &vec) {
    if (vec.empty()) {
      return *this;
    }
    return Insert(vec.begin(), vec.end());
  }

  // Given a span, determine the ranges of possible start and end offsets
  // for each non-terminal.
  void CalculateRanges(int start, int end,
                       std::vector<NonTermRange> &ranges) const {
    ranges.resize(m_rank);
    const VarSpanNode *n = this;
    size_t firstIndex = m_rank;
    while (n->m_parent) {
      const KeyType &key = *(n->m_label);
      assert(key[0] == 0 || key[0] == key[1]);
      assert(key[3] == -1 || key[2] == key[3]);
      const int numSplitPoints = key[4];
      firstIndex -= numSplitPoints+1;
      const int vsn_start = key[0] == 0 ? start : key[0];
      const int vsn_end = key[3] == -1 ? end : key[3];
      // The start position of the first non-terminal is known.
      ranges[firstIndex].s1 = ranges[firstIndex].s2 = vsn_start - start;
      // The end range depends on the number of split points.  If there are
      // no split points then the end position is fixed.
      if (numSplitPoints) {
        ranges[firstIndex].e1 = vsn_start - start;
        ranges[firstIndex].e2 = vsn_end - start - numSplitPoints;
      } else {
        ranges[firstIndex].e1 = ranges[firstIndex].e2 = vsn_end - start;
      }
      // For the remaining non-terminals, the start and end boundaries shift
      // by one position with each split point.
      for (int i = 1; i <= numSplitPoints; ++i) {
        ranges[firstIndex+i].s1 = ranges[firstIndex].s1+i;
        ranges[firstIndex+i].s2 = ranges[firstIndex].e2+i;
        ranges[firstIndex+i].e1 = ranges[firstIndex].s1+i;
        ranges[firstIndex+i].e2 = ranges[firstIndex].e2+i;
      }
      // Except that the end point of the final non-terminal is fixed.
      ranges[firstIndex+numSplitPoints].e1 = vsn_end - start;
      ranges[firstIndex+numSplitPoints].e2 = vsn_end - start;
      n = n->m_parent;
    }
    assert(firstIndex == 0);
  }

  const VarSpanNode *m_parent;
  const KeyType *m_label;
  size_t m_rank;
  MapType m_children;

private:
  VarSpanNode &Insert(NodeVec::const_iterator first,
                      NodeVec::const_iterator last) {
    assert(first != last);

    KeyType key;
    key[0] = first->m_start.first;
    key[1] = first->m_start.second;
    key[2] = first->m_end.first;
    key[3] = first->m_end.second;
    key[4] = first->m_numSplitPoints;

    std::pair<MapType::iterator, bool> result = m_children.insert(
          std::make_pair(key, VarSpanNode()));
    VarSpanNode &child = result.first->second;
    if (result.second) {
      child.m_parent = this;
      child.m_label = &(result.first->first);
      child.m_rank = m_rank + first->m_numSplitPoints + 1;
    }
    if (++first == last) {
      return child;
    }
    return child.Insert(first, last);
  }
};

}
