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

#include "Trie.h"
#include "UTrieNode.h"

namespace Moses
{

class Phrase;
class TargetPhrase;
class TargetPhraseCollection;
class Word;
class ChartParser;

/** Implementation of RuleTableTrie.  A RuleTableUTrie is designed to store
 * string-to-tree SCFG grammars only (i.e. rules can have distinct labels on
 * the target side, but only a generic non-terminal on the source side).
 * A key is the source RHS (one symbol per edge) of a rule and a mapped value
 * is the collection of grammar rules that share the same source RHS.
 *
 * (The 'U' in UTrie stands for 'unlabelled' -- the keys are unlabelled and
 * the target labels are stored on the node values, as opposed to the grammar
 * being a monolingual projection with target labels projected onto the source
 * side.)
 */
class RuleTableUTrie : public RuleTableTrie
{
public:
  RuleTableUTrie(const std::string &line)
    : RuleTableTrie(line) {
  }

  const UTrieNode &GetRootNode() const {
    return m_root;
  }

  ChartRuleLookupManager *CreateRuleLookupManager(const ChartParser &,
      const ChartCellCollectionBase &);

private:
  TargetPhraseCollection &GetOrCreateTargetPhraseCollection(
    const Phrase &source, const TargetPhrase &target, const Word *sourceLHS);

  UTrieNode &GetOrCreateNode(const Phrase &source, const TargetPhrase &target,
                             const Word *sourceLHS);

  void SortAndPrune();

  UTrieNode m_root;
};

}  // namespace Moses
