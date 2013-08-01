//Fabienne Braune
//Implementation of a Phrase Dictionary for l-MBOT rules


#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/InputType.h"
#include "moses/InputFileStream.h"
#include "moses/NonTerminal.h"
#include "PhraseDictionaryNodeSCFG.h"
#include "PhraseDictionaryMBOT.h"

#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemoryMBOT.h"


using namespace std;

namespace Moses
{

/*bool PhraseDictionaryMBOT::Load(const std::vector<FactorType> &input
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
}*/

TargetPhraseCollection &PhraseDictionaryMBOT::GetOrCreateTargetPhraseCollection(
                                                                                const Phrase &source
                                                                                , const TargetPhraseMBOT &target
                                                                                , const Word &sourceLHS)
{
      PhraseDictionaryNodeMBOT &currNode = GetOrCreateNode(source, target, sourceLHS);
      return currNode.GetOrCreateTargetPhraseCollectionMBOT(sourceLHS);
}

//Fabienne Braune : would be cool to overload CreateRuleLookupManager and cast output to ChartRuleLookupManagerMemoryMBOT. Was
//more easy for me to write a new method...
ChartRuleLookupManagerMemory *PhraseDictionaryMBOT::CreateRuleLookupManagerForMBOT(
  const InputType &sentence,
  const ChartCellCollectionBase &cellCollection)
{
  //Fabienne Braune : cast to ChartRuleLookupManagerMemory because of problems with covariant return type.
  std::cerr << "CREATING MBOT RULE LOOKUP MANAGER ..." << std::endl;
  return static_cast<ChartRuleLookupManagerMemory*> (new ChartRuleLookupManagerMemoryMBOT(sentence, cellCollection, *this));
}


PhraseDictionaryNodeMBOT &PhraseDictionaryMBOT::GetOrCreateNode(const Phrase &source
                                                                , const TargetPhraseMBOT &targetMBOT
                                                                , const Word &sourceLHS)
{
  const size_t size = source.GetSize();


  //new : get alignments from target phrase MBOT
   const std::vector<const AlignmentInfoMBOT*> *alignmentsInfo = targetMBOT.GetMBOTAlignments();
   std::vector<const AlignmentInfoMBOT*> :: const_iterator itr_alignments;

  PhraseDictionaryNodeMBOT *currNode = &m_mbotCollection;

  for (size_t pos = 0 ; pos < size ; ++pos) {

    const Word& word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      const Word &sourceNonTerm = word;

      //new : check that source non-terminal is aligned to at least one target non-terminal in the vector
      //1. Check that vector is not empty
      CHECK(alignmentsInfo->size() != 0);
      //2. Check that source word is in vector
      bool isInVector = 0;
      //We represent each target phrase by two vectors (1) alignment in multiple target phrases (MBOT) (2) multiple alignments to same source

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

      WordSequence targetNonTerm;

      targetMBOT.GetWordVector(targetIndices, targetNonTerm);
      currNode = currNode->GetOrCreateChild(sourceNonTerm, targetNonTerm);
    } else {
      //std::cout << "Creating Child for PDN... : " << std::endl;
      currNode = currNode->GetOrCreateChildMBOT(&word);
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
