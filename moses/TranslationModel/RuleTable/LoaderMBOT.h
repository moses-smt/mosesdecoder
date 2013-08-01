//Fabienne Braune
//Loader for l-MBOT rules

#ifndef moses_RuleTableLoaderMBOT_h
#define moses_RuleTableLoaderMBOT_h

#include "LoaderStandard.h"
#include "PhraseDictionaryMBOT.h"

namespace Moses {

class RuleTableLoaderMBOT : public RuleTableLoader
{

public:

  bool Load(const std::vector<FactorType> &input,
              const std::vector<FactorType> &output,
              const std::string &inFile,
              const std::vector<float> &weight,
              size_t tableLimit,
              const LMList &languageModels,
              const WordPenaltyProducer* wpProducer,
              RuleTableTrie &);



//! Overlaod method GetOrCreateTargetPhraseCollection from RuleTable
//! Cast ruleTable to PhraseDictionaryMBOT
TargetPhraseCollection &GetOrCreateTargetPhraseCollection(
		RuleTableTrie &ruleTable
      , const Phrase &source
      , const TargetPhraseMBOT &target
      , const Word &sourceLHS) {

        //new : cast rule table to phrase dictionary MBOT
        PhraseDictionaryMBOT* ruleTableMBOT;
        ruleTableMBOT = dynamic_cast<PhraseDictionaryMBOT*>(&ruleTable);
        return ruleTableMBOT->GetOrCreateTargetPhraseCollection(source,target,sourceLHS);
    }

};

}  // namespace Moses

#endif
