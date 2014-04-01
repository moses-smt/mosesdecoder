// $Id: PhraseDictionaryMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
// vim:tabstop=2

/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2006 University of Edinburgh

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

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include "RuleTableLoader.h"
#include "RuleTableLoaderFactory.h"
#include "PhraseDictionarySCFG.h"
#include "PhraseDictionaryMBOT.h"
#include "RuleTableLoaderMBOT.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "ChartRuleLookupManagerMemoryMBOT.h"

using namespace std;

namespace Moses
{

bool PhraseDictionaryMBOT::Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , const WordPenaltyProducer* wpProducer)
{

  m_filePath = filePath;
  m_tableLimit = tableLimit;


  // data from file
  InputFileStream inFile(filePath);

  std::auto_ptr<RuleTableLoader> loader =
  RuleTableLoaderFactory::Create(filePath);
  bool ret = loader->Load(input, output, inFile, weight, tableLimit,
                          languageModels, wpProducer, *this);
  return ret;
}

TargetPhraseCollection &PhraseDictionaryMBOT::GetOrCreateTargetPhraseCollection(
                                                                                const Phrase &source
                                                                                , const TargetPhraseMBOT &target
                                                                                , const Word &sourceLHS)
{
  PhraseDictionaryNodeMBOT &currNode = GetOrCreateNode(source, target, sourceLHS);
  return currNode.GetOrCreateTargetPhraseCollectionMBOT(sourceLHS);
}


ChartRuleLookupManagerMemoryMBOT *PhraseDictionaryMBOT::CreateRuleLookupManager(
  const InputType &sentence,
  const ChartCellCollection &cellCollection)
{
  return new ChartRuleLookupManagerMemoryMBOT(sentence, cellCollection, *this);
}


PhraseDictionaryNodeMBOT &PhraseDictionaryMBOT::GetOrCreateNode(const Phrase &source
                                                                , const TargetPhraseMBOT &targetMBOT
                                                                , const Word &sourceLHS)
{

   const size_t size = source.GetSize();
   const std::vector<const AlignmentInfoMBOT*> *alignmentsInfo = targetMBOT.GetMBOTAlignments();
   std::vector<const AlignmentInfoMBOT*> :: const_iterator itr_alignments;

   PhraseDictionaryNodeMBOT *currNode = &m_mbotCollection;
   for (size_t pos = 0 ; pos < size ; ++pos) {

    const Word& word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      // indexed by source label 1st
      const Word &sourceNonTerm = word;

      //1. Check that vector is not empty
      CHECK(alignmentsInfo->size() != 0);
      //2. Check that source word is in vector
      bool isInVector = 0;
      //3. We represent each target phrase by a vector where
      //the indices in the vector correspond to the position of the target phrase
      std::vector<size_t> emptyVector;
      std::vector<std::vector <size_t> > targetIndices(alignmentsInfo->size(), emptyVector);

      int counter = 0;
      for(itr_alignments = alignmentsInfo->begin(); itr_alignments != alignmentsInfo->end(); itr_alignments++)
      {
          std::vector<size_t> insideAlignments;
          //3. Check that no field in vector is empty
          bool isEmpty = 1;
          const AlignmentInfoMBOT *current = *itr_alignments;
          AlignmentInfoMBOT :: const_iterator itr_align;
          for(itr_align = current->begin(); itr_align != current->end(); itr_align++)
          {
                if(itr_align->first == pos)
                {
                    isInVector = 1;
                    size_t targetNonTermInd = itr_align->second;
                    targetIndices[counter].push_back(targetNonTermInd);
                }
            }
            if(itr_align != current->end())
            {
              isEmpty = 0;
            }
            CHECK(isEmpty == 1);
            counter++;
      }
      CHECK(isInVector == 1);

      std::vector<Word> targetNonTerm;
      targetMBOT.GetWordVector(targetIndices, targetNonTerm);

      currNode = currNode->GetOrCreateChild(sourceNonTerm, targetNonTerm);
    } else {
      currNode = currNode->GetOrCreateChildMBOT(word);
    }
  CHECK(currNode != NULL);
  }
  return *currNode;
}

void PhraseDictionaryMBOT::SortAndPrune()
{
  if (GetTableLimit())
  {
    m_mbotCollection.SortMBOT(GetTableLimit());
  }
}


} //end of namespace
