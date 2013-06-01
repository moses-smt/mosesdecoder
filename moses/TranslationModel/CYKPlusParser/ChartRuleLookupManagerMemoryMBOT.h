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
#include "DotChartInMemoryMBOT.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellMBOT.h"
#include "ChartRuleLookupManagerMemory.h"
#include "ChartRuleLookupManagerShallowMBOT.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryNodeMBOT.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryMBOT.h"
#include "moses/StackVec.h"


namespace Moses
{

class ChartTranslationOptionList;
class DottedRuleColl;
class WordsRange;

// Implementation of ChartRuleLookupManager for in-memory rule tables.
//Fabienne Braune : not sure if it is a good idea to use mutiple inheritance here...
class ChartRuleLookupManagerMemoryMBOT : public ChartRuleLookupManagerMemory, public ChartRuleLookupManagerShallowMBOT

{
public:
  ChartRuleLookupManagerMemoryMBOT(const InputType &sentence,
                               const ChartCellCollectionBase &cellColl,
                               const PhraseDictionaryMBOT &ruleTable);

  ~ChartRuleLookupManagerMemoryMBOT();

  //Fabienne Braune : we get this one from non-l-mbot ChartRuleLookupManager. Crash in case it gets called
  virtual void GetChartRuleCollection(
    const WordsRange &range,
    ChartParserCallback &outColl)
  {
	  std::cerr << "Chart rule lookup for l-mbot rules does not support chart rules" << std::endl;
	  abort();
  };

  virtual void GetMBOTRuleCollection(
    const WordsRange &range,
    ChartParserCallback &outColl);


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
