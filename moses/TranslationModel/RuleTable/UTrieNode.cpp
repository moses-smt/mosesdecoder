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

#include "moses/NonTerminal.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/Util.h"
#include "moses/Word.h"
#include "UTrieNode.h"
#include "Trie.h"
#include "moses/TranslationModel/PhraseDictionaryNodeMemory.h"  // For TerminalHasher and TerminalEqualityPred

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#include <map>
#include <vector>

namespace Moses
{

void UTrieNode::Prune(size_t tableLimit)
{
  // Recusively prune child node values.
  for (TerminalMap::iterator p = m_terminalMap.begin();
       p != m_terminalMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }
  if (m_gapNode) {
    m_gapNode->Prune(tableLimit);
  }

  // Prune TargetPhraseCollections at this node.
  for (LabelMap::iterator p = m_labelMap.begin(); p != m_labelMap.end(); ++p) {
    p->second.Prune(true, tableLimit);
  }
}

void UTrieNode::Sort(size_t tableLimit)
{
  // Recusively sort child node values.
  for (TerminalMap::iterator p = m_terminalMap.begin();
       p != m_terminalMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }
  if (m_gapNode) {
    m_gapNode->Sort(tableLimit);
  }

  // Sort TargetPhraseCollections at this node.
  for (LabelMap::iterator p = m_labelMap.begin(); p != m_labelMap.end(); ++p) {
    p->second.Sort(true, tableLimit);
  }
}

UTrieNode *UTrieNode::GetOrCreateTerminalChild(const Word &sourceTerm)
{
  assert(!sourceTerm.IsNonTerminal());
  std::pair<TerminalMap::iterator, bool> result;
  result = m_terminalMap.insert(std::make_pair(sourceTerm, UTrieNode()));
  const TerminalMap::iterator &iter = result.first;
  UTrieNode &child = iter->second;
  return &child;
}

UTrieNode *UTrieNode::GetOrCreateNonTerminalChild(const Word &targetNonTerm)
{
  assert(targetNonTerm.IsNonTerminal());
  if (m_gapNode == NULL) {
    m_gapNode = new UTrieNode();
  }
  return m_gapNode;
}

TargetPhraseCollection &UTrieNode::GetOrCreateTargetPhraseCollection(
  const TargetPhrase &target)
{
  const AlignmentInfo &alignmentInfo = target.GetAlignNonTerm();
  const size_t rank = alignmentInfo.GetSize();

  std::vector<int> vec;
  vec.reserve(rank);

  m_labelTable.resize(rank);

  int i = 0;
  for (AlignmentInfo::const_iterator p = alignmentInfo.begin();
       p != alignmentInfo.end(); ++p) {
    size_t targetNonTermIndex = p->second;
    const Word &targetNonTerm = target.GetWord(targetNonTermIndex);
    vec.push_back(InsertLabel(i++, targetNonTerm));
  }

  return m_labelMap[vec];
}

}  // namespace Moses
