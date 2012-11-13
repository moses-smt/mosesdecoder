/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh
 
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

#include "moses/PhraseDictionary.h"
#include "moses/InputType.h"
#include "moses/NonTerminal.h"
#include "PhraseDictionaryNodeSCFG.h"
#include "Trie.h"

namespace Moses
{

/** Implementation of a SCFG rule table in a trie.  Looking up a rule of
 * length n symbols requires n look-ups to find the TargetPhraseCollection.
 */
class PhraseDictionarySCFG : public RuleTableTrie
{
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionarySCFG&);
  friend class RuleTableLoader;

 public:
  PhraseDictionarySCFG(size_t numScoreComponents,
                       PhraseDictionaryFeature* feature)
      : RuleTableTrie(numScoreComponents, feature) {}

  const PhraseDictionaryNodeSCFG &GetRootNode() const { return m_collection; }

  ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &);

  TO_STRING();

 protected:
  TargetPhraseCollection &GetOrCreateTargetPhraseCollection(
      const Phrase &source, const TargetPhrase &target, const Word &sourceLHS);

  PhraseDictionaryNodeSCFG &GetOrCreateNode(const Phrase &source
                                            , const TargetPhrase &target
                                            , const Word &sourceLHS);

  void SortAndPrune();

  PhraseDictionaryNodeSCFG m_collection;
};

}  // namespace Moses
