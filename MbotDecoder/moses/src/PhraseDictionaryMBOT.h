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

#ifndef moses_PhraseDictionaryMBOT_h
#define moses_PhraseDictionaryMBOT_h

#include "PhraseDictionarySCFG.h"
#include "PhraseDictionaryNodeMBOT.h"

#include "ChartRuleLookupManagerMemoryMBOT.h"

namespace Moses
{

class PhraseDictionaryMBOT : public PhraseDictionarySCFG
{
    friend class RuleTableLoaderMBOT;

 public:

    PhraseDictionaryMBOT(size_t numScoreComponents,
    PhraseDictionaryFeature* feature)
    : PhraseDictionarySCFG(numScoreComponents, feature) {}


    bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , const WordPenaltyProducer* wpProducer);


ChartRuleLookupManagerMemoryMBOT *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollection &);

  const PhraseDictionaryNodeSCFG &GetRootNode() const
  {
    std::cout << "Get root node of non mbot phrase dictionary NOT IMPLEMENTED in phrase dictionary MBOT" << std::endl;
  }

  const PhraseDictionaryNodeMBOT &GetRootNodeMBOT() const
  {
    //std::cout << "Getting root node from " << m_mbotCollection << std::endl;
    return m_mbotCollection;
  }

  void SortAndPrune();

 protected:

  TargetPhraseCollection &GetOrCreateTargetPhraseCollection(const Phrase &source, const TargetPhraseMBOT &target, const Word &sourceLHS);

  PhraseDictionaryNodeMBOT &GetOrCreateNode(const Phrase &source
                                            , const TargetPhraseMBOT &target
                                            , const Word &sourceLHS);

 //void InitializeForInput(InputType const& source);

  PhraseDictionaryNodeMBOT m_mbotCollection;
  std::string m_filePath;

};

}  // namespace Moses

#endif

