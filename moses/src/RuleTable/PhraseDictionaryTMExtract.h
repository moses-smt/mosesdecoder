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

#include "PhraseDictionary.h"
#include "PhraseDictionaryNodeSCFG.h"
#include "PhraseDictionarySCFG.h"
#include "InputType.h"
#include "NonTerminal.h"
#include "RuleTable/Trie.h"

namespace Moses
{
  
  /** Implementation of a SCFG rule table in a trie.  Looking up a rule of
   * length n symbols requires n look-ups to find the TargetPhraseCollection.
   */
  class PhraseDictionaryTMExtract : public PhraseDictionary
  {
    friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryTMExtract&);
    friend class RuleTableLoader;
    
  public:
    PhraseDictionaryTMExtract(size_t numScoreComponents,
                              PhraseDictionaryFeature* feature);
    
    void Initialize(const std::string &initStr);

    const PhraseDictionaryNodeSCFG &GetRootNode(const InputType &source) const;
    
    ChartRuleLookupManager *CreateRuleLookupManager(
                                                    const InputType &,
                                                    const ChartCellCollection &);
    void InitializeForInput(InputType const& source);
    void CleanUp(const InputType& source);
    
    virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const
    {}
    virtual DecodeType GetDecodeType() const
    {}
    
    TO_STRING();
    
  protected:
    TargetPhraseCollection &GetOrCreateTargetPhraseCollection(const InputType &inputSentence
                                                              , const Phrase &source
                                                              , const TargetPhrase &target
                                                              , const Word &sourceLHS);
    
    PhraseDictionaryNodeSCFG &GetOrCreateNode(const InputType &inputSentence
                                              , const Phrase &source
                                              , const TargetPhrase &target
                                              , const Word &sourceLHS);
    
    void SortAndPrune(const InputType &source);
    PhraseDictionaryNodeSCFG &GetRootNode(const InputType &source);

    std::map<long, PhraseDictionaryNodeSCFG> m_collection;
    std::vector<std::string> m_config;
  };
  
}  // namespace Moses
