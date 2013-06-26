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

#include "moses/NonTerminal.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/Terminal.h"
#include "moses/Util.h"
#include "moses/Word.h"
#include "Trie.h"

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#include <map>
#include <vector>

namespace Moses
{

class RuleTableUTrie;

//! @todo ask phil williams - whats the diff between this and phrasedictionaryNode
class UTrieNode
{
public:
  typedef std::vector<std::vector<Word> > LabelTable;
#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_map<Word,
          UTrieNode,
          TerminalHasher,
          TerminalEqualityPred> TerminalMap;

  typedef boost::unordered_map<std::vector<int>,
          TargetPhraseCollection> LabelMap;
#else
  typedef std::map<Word, UTrieNode> TerminalMap;
  typedef std::map<std::vector<int>, TargetPhraseCollection> LabelMap;
#endif

  ~UTrieNode() {
    delete m_gapNode;
  }

  const LabelTable &GetLabelTable() const {
    return m_labelTable;
  }
  const LabelMap &GetLabelMap() const {
    return m_labelMap;
  }
  const TerminalMap &GetTerminalMap() const {
    return m_terminalMap;
  }

  const UTrieNode *GetNonTerminalChild() const {
    return m_gapNode;
  }

  UTrieNode *GetOrCreateTerminalChild(const Word &sourceTerm);
  UTrieNode *GetOrCreateNonTerminalChild(const Word &targetNonTerm);

  TargetPhraseCollection &GetOrCreateTargetPhraseCollection(
    const TargetPhrase &);

  bool IsLeaf() const {
    return m_terminalMap.empty() && m_gapNode == NULL;
  }

  bool HasRules() const {
    return !m_labelMap.empty();
  }

  void Prune(size_t tableLimit);
  void Sort(size_t tableLimit);

private:
  friend class RuleTableUTrie;

  UTrieNode() : m_gapNode(NULL) {}

  int InsertLabel(int i, const Word &w) {
    std::vector<Word> &inner = m_labelTable[i];
    for (size_t j = 0; j < inner.size(); ++j) {
      if (inner[j] == w) {
        return j;
      }
    }
    inner.push_back(w);
    return inner.size()-1;
  }

  LabelTable m_labelTable;
  LabelMap m_labelMap;
  TerminalMap m_terminalMap;
  UTrieNode *m_gapNode;
};

}  // namespace Moses
