/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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

#include "ChartRuleLookupManagerCYKPlus.h"
#include "DotChartInMemory.h"

#include "moses/InputType.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"
#include "moses/ChartParserCallback.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"

namespace Moses
{

void ChartRuleLookupManagerCYKPlus::AddCompletedRule(
  const DottedRule &dottedRule,
  const TargetPhraseCollection &tpc,
  const Range &range,
  ChartParserCallback &outColl)
{
  // Determine the rule's rank.
  size_t rank = 0;
  const DottedRule *node = &dottedRule;
  while (!node->IsRoot()) {
    if (node->IsNonTerminal()) {
      ++rank;
    }
    node = node->GetPrev();
  }

  // Fill m_stackVec with a stack pointer for each non-terminal.
  m_stackVec.resize(rank);
  node = &dottedRule;
  while (rank > 0) {
    if (node->IsNonTerminal()) {
      m_stackVec[--rank] = &node->GetChartCellLabel();
    }
    node = node->GetPrev();
  }

  // Add the (TargetPhraseCollection, StackVec) pair to the collection.
  outColl.Add(tpc, m_stackVec, range);
}

}  // namespace Moses
