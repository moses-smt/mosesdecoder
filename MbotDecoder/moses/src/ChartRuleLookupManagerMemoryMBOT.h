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
#ifndef moses_ChartRuleLookupManagerMemoryMBOT_h
#define moses_ChartRuleLookupManagerMemoryMBOT_h

#include <vector>

#if HAVE_CONFIG_H
#include "config.h"
#ifdef USE_BOOST_POOL
#include <boost/pool/object_pool.hpp>
#endif
#endif

#include "DotChartInMemory.h"
#include "NonTerminal.h"
#include "PhraseDictionaryNodeSCFG.h"
#include "PhraseDictionarySCFG.h"
#include "ChartCellMBOT.h"
#include "ChartRuleLookupManagerMemory.h"

namespace Moses
{

class ChartTranslationOptionList;
class DottedRuleColl;
class WordsRange;

// Implementation of ChartRuleLookupManager for in-memory rule tables.
class ChartRuleLookupManagerMemoryMBOT : public ChartRuleLookupManagerMemory
{
public:
  ChartRuleLookupManagerMemoryMBOT(const InputType &sentence,
                               const ChartCellCollection &cellColl,
                               const PhraseDictionaryMBOT &ruleTable);

  ~ChartRuleLookupManagerMemoryMBOT();

  virtual void GetChartRuleCollection(
    const WordsRange &range,
    bool adhereTableLimit,
    ChartTranslationOptionList &outColl);


private:
  void ExtendPartialRuleApplicationMBOT(
    const DottedRuleInMemoryMBOT &prevDottedRule,
    size_t startPos,
    size_t endPos,
    size_t stackInd,
    DottedRuleCollMBOT &dottedRuleColl);

  std::vector<DottedRuleCollMBOT*> m_mbotDottedRuleColls;
  const PhraseDictionaryMBOT &m_mbotRuleTable;
#ifdef USE_BOOST_POOL
  // Use an object pool to allocate the dotted rules for this sentence.  We
  // allocate a lot of them and this has been seen to significantly improve
  // performance, especially for multithreaded decoding.
  boost::object_pool<DottedRuleInMemoryMBOT> m_mbotDottedRulePool;
#endif
};

}  // namespace Moses

#endif
