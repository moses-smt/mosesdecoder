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

#pragma once
#ifndef moses_ChartRuleLookupManagerCYKPlus_h
#define moses_ChartRuleLookupManagerCYKPlus_h

#include "moses/ChartRuleLookupManager.h"
#include "moses/StackVec.h"

namespace Moses
{

class DottedRule;
class TargetPhraseCollection;
class Range;

/** @todo what is this?
 */
class ChartRuleLookupManagerCYKPlus : public ChartRuleLookupManager
{
public:
  ChartRuleLookupManagerCYKPlus(const ChartParser &parser,
                                const ChartCellCollectionBase &cellColl)
    : ChartRuleLookupManager(parser, cellColl) {}

protected:
  void AddCompletedRule(
    const DottedRule &dottedRule,
    const TargetPhraseCollection &tpc,
    const Range &range,
    ChartParserCallback &outColl);

  StackVec m_stackVec;
};

// struct that caches cellLabel, its end position and score for quicker lookup
struct ChartCellCache {
  ChartCellCache(size_t endPos, const ChartCellLabel* cellLabel, float score)
    : endPos(endPos)
    , cellLabel(cellLabel)
    , score(score) {}

  size_t endPos;
  const ChartCellLabel* cellLabel;
  float score;
};

}  // namespace Moses

#endif
