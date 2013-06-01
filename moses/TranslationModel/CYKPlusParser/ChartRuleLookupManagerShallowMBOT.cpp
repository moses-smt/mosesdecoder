#include "ChartRuleLookupManagerShallowMBOT.h"
#include "DotChartInMemory.h"
#include "DotChartInMemoryMBOT.h"

#include "moses/TranslationModel/RuleTable/PhraseDictionarySCFG.h"
#include "moses/InputType.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"
#include "moses/ChartParserCallback.h"

namespace Moses
{

void ChartRuleLookupManagerShallowMBOT::AddCompletedRuleMBOT(
  const DottedRuleMBOT &dottedRule,
  const TargetPhraseCollection &tpc,
  const WordsRange &range,
  ChartParserCallback &outColl)
{
  // Determine the rule's rank.
  size_t rank = 0;
  const DottedRuleMBOT *node = &dottedRule;
  while (!node->IsRootMBOT()) {
    if (node->IsNonTerminalMBOT()) {
      ++rank;
    }
    node = node->GetPrevMBOT();
  }

  // Fill m_stackVec with a stack pointer for each non-terminal.
  m_stackVec.resize(rank);
  node = &dottedRule;
  while (rank > 0) {
    if (node->IsNonTerminalMBOT()) {
      m_stackVec[--rank] = node->GetChartCellLabelMBOT();
    }
    node = node->GetPrevMBOT();
  }

  // Add the (TargetPhraseCollection, StackVec) pair to the collection.
  outColl.Add(tpc, m_stackVec, range);
}

}  // namespace Moses
