// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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

#include "Trie.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/InputType.h"
#include "moses/NonTerminal.h"
#include "moses/TranslationModel/fuzzy-match/FuzzyMatchWrapper.h"
#include "moses/TranslationModel/PhraseDictionaryNodeMemory.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"

namespace Moses
{
class PhraseDictionaryNodeMemory;
class ChartParser;

/** Implementation of a SCFG rule table in a trie.  Looking up a rule of
 * length n symbols requires n look-ups to find the TargetPhraseCollection.
 */
class PhraseDictionaryFuzzyMatch : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryFuzzyMatch&);
  friend class RuleTableLoader;

public:
  PhraseDictionaryFuzzyMatch(const std::string &line);
  ~PhraseDictionaryFuzzyMatch();
  void Load(AllOptions::ptr const& opts);

  const PhraseDictionaryNodeMemory &GetRootNode(long translationId) const;

  ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &parser,
    const ChartCellCollectionBase &,
    std::size_t);
  void InitializeForInput(ttasksptr const& ttask);
  void CleanUpAfterSentenceProcessing(const InputType& source);

  void SetParameter(const std::string& key, const std::string& value);

  TO_STRING();

protected:
  TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection(PhraseDictionaryNodeMemory &rootNode
                                    , const Phrase &source
                                    , const TargetPhrase &target
                                    , const Word *sourceLHS);

  PhraseDictionaryNodeMemory &GetOrCreateNode(PhraseDictionaryNodeMemory &rootNode
      , const Phrase &source
      , const TargetPhrase &target
      , const Word *sourceLHS);

  void SortAndPrune(PhraseDictionaryNodeMemory &rootNode);
  PhraseDictionaryNodeMemory &GetRootNode(const InputType &source);

  std::map<long, PhraseDictionaryNodeMemory> m_collection;
  std::vector<std::string> m_config;

  tmmt::FuzzyMatchWrapper *m_FuzzyMatchWrapper;

};

}  // namespace Moses
