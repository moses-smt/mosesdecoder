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
  //new : inserted for testing
  //std::cout << "CREATE TARGET PHRASE COLL IN PDMBOT " << std::endl;

  //THAT WAS WRONG !!!
  PhraseDictionaryNodeMBOT &currNode = GetOrCreateNode(source, target, sourceLHS);
  //std::cout << "NODE BEFORE CREATION" << &currNode << std::endl;
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

    //std::cout << "PDMBOT : Creating Node... " << std::endl;

  const size_t size = source.GetSize();

    //std::cout << "Size of source " << size << std::endl;

  //new : get alignments from target phrase MBOT
   const std::vector<const AlignmentInfoMBOT*> *alignmentsInfo = targetMBOT.GetMBOTAlignments();
   std::vector<const AlignmentInfoMBOT*> :: const_iterator itr_alignments;

    //original : const AlignmentInfo &alignmentInfo = target.GetAlignmentInfo();

  //new : inserted for testing
  //std::cout << "Alignment info : " << alignmentInfo << std::endl;

  //TODO: REWRITE ITERATOR
  //AlignmentInfo::const_iterator iterAlign = alignmentInfo.begin();

  PhraseDictionaryNodeMBOT *currNode = &m_mbotCollection;
  //std::cout << "Getted node is AT : " << currNode << std::endl;

  for (size_t pos = 0 ; pos < size ; ++pos) {

      //std::cout << "POS TO FIND : " << pos << std::endl;
    const Word& word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      // indexed by source label 1st
      const Word &sourceNonTerm = word;

      //new : inserted for testing
      //std::cout << "Source Non Terminal Word is "<< word << std::endl;

      //new : check that it is aligned to at least one target non-terminal in the vector
      //1. Check that vector is not empty
      CHECK(alignmentsInfo->size() != 0);
      //2. Check that source word is in vector
      bool isInVector = 0;
      //3. We represent each target phrase by a vector where
      //the indices in the vector correspond to the position of the target phrase
      //FB : BEWARE : TO MAKE BETTER : (make vector of vector) : two vectors (1) alignment in multiple target phrases (MBOT) (2) multiple alignments to same source
      //Vector 1 : alignments in multiple target phrases
      std::vector<size_t> emptyVector;
      std::vector<std::vector <size_t> > targetIndices(alignmentsInfo->size(), emptyVector);

      //std::cout << "Size of Target non term ind 1 :"<< targetIndices.size() << std::endl;
      int counter = 0;
      for(itr_alignments = alignmentsInfo->begin(); itr_alignments != alignmentsInfo->end(); itr_alignments++)
      {
          //std::cout << "IN LOOP : " << counter << std::endl;
          std::vector<size_t> insideAlignments;
          //3. Check that no field in vector is empty
          bool isEmpty = 1;
          const AlignmentInfoMBOT *current = *itr_alignments;
          AlignmentInfoMBOT :: const_iterator itr_align;
          for(itr_align = current->begin(); itr_align != current->end(); itr_align++)
          {
              //std::cout << "FOUND POSITION : " << itr_align->first << std::endl;
              //std::cout << "CURRENT POSITION : " << pos <<  std::endl;
              //checks
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

      //new : inserted for testing
      //std::cout << "PDSCFG Before getting word"<< std::endl;
      //std::cout << "Size of Target non term ind 2 :"<< targetIndices.size() << std::endl;

      //remove duplicates
      //std::sort(vec.begin(), vec.end());
      //vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

      std::vector<Word> targetNonTerm;
      targetMBOT.GetWordVector(targetIndices, targetNonTerm);

      currNode = currNode->GetOrCreateChild(sourceNonTerm, targetNonTerm);
    } else {
      //std::cout << "Creating Child for PDN... : " << std::endl;
      currNode = currNode->GetOrCreateChildMBOT(word);
    }
  CHECK(currNode != NULL);
  }  // finally, the source LHS
  //currNode = currNode->GetOrCreateChild(sourceLHS);
  //CHECK(currNode != NULL);
  //PhraseDictionaryNodeMBOT currNodeT = *currNode;
  //std::cout << "Found node is At : " << currNode << "TPC" << currNodeT.GetTargetPhraseCollection() << std::endl;
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
