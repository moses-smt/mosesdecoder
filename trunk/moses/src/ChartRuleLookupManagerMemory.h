/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2011 University of Edinburgh

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
#ifndef moses_ChartRuleLookupManagerMemory_h
#define moses_ChartRuleLookupManagerMemory_h

#include <vector>

#include "ChartRuleLookupManager.h"
#include "NonTerminal.h"
#include "PhraseDictionaryNodeSCFG.h"
#include "PhraseDictionarySCFG.h"

namespace Moses
{

class ChartTranslationOptionList;
class ProcessedRuleColl;
class WordsRange;
class WordConsumed;

// Implementation of ChartRuleLookupManager for in-memory rule tables.
class ChartRuleLookupManagerMemory : public ChartRuleLookupManager
{
 public:
  ChartRuleLookupManagerMemory(const InputType &sentence,
                               const CellCollection &cellColl,
                               const PhraseDictionarySCFG &ruleTable);

  ~ChartRuleLookupManagerMemory();

  virtual void GetChartRuleCollection(
      const WordsRange &range,
      bool adhereTableLimit,
      ChartTranslationOptionList &outColl);

 private:
  void ExtendPartialRuleApplication(
      const PhraseDictionaryNodeSCFG &node,
      const WordConsumed *prevWordConsumed,
      size_t startPos,
      size_t endPos,
      size_t stackInd,
      const NonTerminalSet &sourceNonTerms,
      const NonTerminalSet &targetNonTerms,
      ProcessedRuleColl &processedRuleColl);

  std::vector<ProcessedRuleColl*> m_processedRuleColls;
  const PhraseDictionarySCFG &m_ruleTable;
};

}  // namespace Moses

#endif
