//Fabienne Braune : Lookup manager for l-MBOT rules

#pragma once
#ifndef moses_ChartRuleLookupManagerShallowMBOT_h
#define moses_ChartRuleLookupManagerShallowMBOT_h

#include "moses/ChartRuleLookupManagerMBOT.h"
#include "moses/StackVec.h"

namespace Moses
{

class DottedRuleMBOT;
class TargetPhraseCollection;
class WordsRange;

/** @todo what is this?
 */
class ChartRuleLookupManagerShallowMBOT : public ChartRuleLookupManagerMBOT
{
 public:
  ChartRuleLookupManagerShallowMBOT(const InputType &sentence,
                                const ChartCellCollectionBase &cellColl)
    : ChartRuleLookupManagerMBOT(sentence, cellColl) {}

 protected:

  void AddCompletedRuleMBOT(
      const DottedRuleMBOT &dottedRule,
      const TargetPhraseCollection &tpc,
      const WordsRange &range,
      ChartParserCallback &outColl);

  StackVec m_stackVec;
};

}  // namespace Moses

#endif
