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

#include "ApplicableRuleTrie.h"

namespace Moses
{

void ApplicableRuleTrie::Extend(const UTrieNode &root, int minPos,
                                const SentenceMap &sentMap, bool followsGap)
{
  const UTrieNode::TerminalMap &termMap = root.GetTerminalMap();
  for (UTrieNode::TerminalMap::const_iterator p = termMap.begin();
       p != termMap.end(); ++p) {
    const Word &word = p->first;
    const UTrieNode &child = p->second;
    SentenceMap::const_iterator q = sentMap.find(word);
    if (q == sentMap.end()) {
      continue;
    }
    for (std::vector<size_t>::const_iterator r = q->second.begin();
         r != q->second.end(); ++r) {
      size_t index = *r;
      if (index == (size_t)minPos || (followsGap && index > (size_t)minPos) || minPos == -1) {
        ApplicableRuleTrie *subTrie = new ApplicableRuleTrie(index, index,
            child);
        subTrie->Extend(child, index+1, sentMap, false);
        m_children.push_back(subTrie);
      }
    }
  }

  const UTrieNode *child = root.GetNonTerminalChild();
  if (!child) {
    return;
  }
  int start = followsGap ? -1 : minPos;
  ApplicableRuleTrie *subTrie = new ApplicableRuleTrie(start, -1, *child);
  int newMinPos = (minPos == -1 ? 1 : minPos+1);
  subTrie->Extend(*child, newMinPos, sentMap, true);
  m_children.push_back(subTrie);
}

}  // namespace Moses
