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

#include "StackLatticeBuilder.h"

#include "moses/ChartRuleLookupManager.h"
#include "moses/TranslationModel/RuleTable/UTrieNode.h"
#include "StackLattice.h"
#include "VarSpanNode.h"

namespace Moses
{

void StackLatticeBuilder::Build(
  int start,
  int end,
  const UTrieNode &ruleNode,
  const VarSpanNode &varSpanNode,
  const std::vector<VarSpanNode::NonTermRange> &ranges,
  const ChartRuleLookupManager &manager,
  StackLattice &lattice,
  std::vector<std::vector<bool> > &checkTable)
{
  // Extend the lattice if necessary.  Do not shrink it.
  const size_t span = end - start + 1;
  if (lattice.size() < span) {
    lattice.resize(span);
  }

  // Extend the quick-check table if necessary.  Do not shrink it.
  if (checkTable.size() < varSpanNode.m_rank) {
    checkTable.resize(varSpanNode.m_rank);
  }

  const UTrieNode::LabelTable &labelTable = ruleNode.GetLabelTable();

  for (size_t index = 0; index < ranges.size(); ++index) {
    const VarSpanNode::NonTermRange &range = ranges[index];
    const std::vector<Word> &labelVec = labelTable[index];
    checkTable[index].clear();
    checkTable[index].resize(labelVec.size(), false);
    // Note: values in range are offsets not absolute positions.
    for (size_t offset = range.s1; offset <= range.s2; ++offset) {
      // Allocate additional space if required.
      if (lattice[offset].size() < index+1) {
        lattice[offset].resize(index+1);
      }
      size_t e1 = std::max(offset, range.e1);
      const size_t maxSpan = range.e2-offset+1;
      if (lattice[offset][index].size() < maxSpan+1) {
        lattice[offset][index].resize(maxSpan+1);
      }
      for (size_t end = e1; end <= range.e2; ++end) {
        const size_t span = end-offset+1;
        // Fill the StackVec at lattice[offset][index][span] by iterating over
        // labelTable[index] and looking up each label over the span
        // [start, end]
        StackVec &stackVec = lattice[offset][index][span];
        stackVec.clear();
        stackVec.reserve(labelVec.size());
        std::vector<bool>::iterator q = checkTable[index].begin();
        for (std::vector<Word>::const_iterator p = labelVec.begin();
             p != labelVec.end(); ++p) {
          const Word &label = *p;
          const ChartCellLabel *stack = manager.GetTargetLabelSet(start+offset, start+offset+span-1).Find(label);
          stackVec.push_back(stack);
          *q++ = *q || static_cast<bool>(stack);
        }
      }
    }
  }
}

}
