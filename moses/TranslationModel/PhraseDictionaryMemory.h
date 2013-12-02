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

#include "PhraseDictionaryNodeMemory.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/InputType.h"
#include "moses/NonTerminal.h"
#include "moses/TranslationModel/RuleTable/Trie.h"

namespace Moses
{
class ChartParser;

/** Implementation of a in-memory rule table in a trie.  Looking up a rule of
 * length n symbols requires n look-ups to find the TargetPhraseCollection.
 */
class PhraseDictionaryMemory : public RuleTableTrie
{
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryMemory&);
  friend class RuleTableLoader;

protected:
  PhraseDictionaryMemory(int type, const std::string &line)
    : RuleTableTrie(line) {
  }

public:
  PhraseDictionaryMemory(const std::string &line);

  const PhraseDictionaryNodeMemory &GetRootNode() const {
    return m_collection;
  }

  ChartRuleLookupManager*
  CreateRuleLookupManager(
    const ChartParser &,
    const ChartCellCollectionBase &);

  // only used by multi-model phrase table, and other meta-features
  const TargetPhraseCollection *GetTargetPhraseCollectionLEGACY(const Phrase& src) const;
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  TO_STRING();

protected:
  TargetPhraseCollection &GetOrCreateTargetPhraseCollection(
    const Phrase &source, const TargetPhrase &target, const Word *sourceLHS);

  PhraseDictionaryNodeMemory &GetOrCreateNode(const Phrase &source
      , const TargetPhrase &target
      , const Word *sourceLHS);

  void SortAndPrune();

  PhraseDictionaryNodeMemory m_collection;
};

}  // namespace Moses
