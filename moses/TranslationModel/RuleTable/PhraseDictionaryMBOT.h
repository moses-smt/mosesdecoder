//Fabienne Braune
//Phrase Dictionary for l-MBOT rules

#ifndef moses_PhraseDictionaryMBOT_h
#define moses_PhraseDictionaryMBOT_h

#include "PhraseDictionarySCFG.h"
#include "PhraseDictionaryNodeMBOT.h"

#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerMemoryMBOT.h"

namespace Moses
{

class ChartRuleLookupManagerMemoryMBOT;

class PhraseDictionaryMBOT : public PhraseDictionarySCFG
{
    friend class RuleTableLoaderMBOT;

 public:

    PhraseDictionaryMBOT(size_t numScoreComponents,
    PhraseDictionaryFeature* feature)
    : PhraseDictionarySCFG(numScoreComponents, feature) {}


   /* bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , const WordPenaltyProducer* wpProducer);*/

    //Fabienne Braune : we get this one from non-l-mbot ChartRuleLookupManager. Crash in case it gets called
    ChartRuleLookupManagerMemory *CreateRuleLookupManager(
    const InputType &input,
    const ChartCellCollectionBase &cellColl)
    {
    	//Fabienne Braune Hack. TODO : Pass ChartCellCollection as a pointer everywhere.
    	return CreateRuleLookupManagerForMBOT(input,cellColl);
    }

    ChartRuleLookupManagerMemory *CreateRuleLookupManagerForMBOT(
       const InputType &,
       const ChartCellCollectionBase &cellColl);

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

