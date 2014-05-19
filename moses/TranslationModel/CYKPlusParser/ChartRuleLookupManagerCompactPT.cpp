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

#include <iostream>
#include "ChartRuleLookupManagerCompactPT.h"

#include "moses/ChartParser.h"
#include "moses/InputType.h"
#include "moses/ChartParserCallback.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"

using namespace std;

namespace Moses
{

ChartRuleLookupManagerCompactPT::ChartRuleLookupManagerCompactPT(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellColl,
  const PhraseDictionaryCompact &ruleTable)
  : ChartRuleLookupManagerCYKPlus(parser, cellColl)
  , m_ruleTable(ruleTable)
{

  size_t sourceSize = parser.GetSize();

  m_completedRules.resize(sourceSize);
  m_activeChart.resize(sourceSize);

}

void ChartRuleLookupManagerCompactPT::GetChartRuleCollection(
  const WordsRange &range,
  size_t lastPos,
  ChartParserCallback &outColl)
{
  size_t startPos = range.GetStartPos();
  size_t endPos = range.GetEndPos();
  size_t width = range.GetNumWordsCovered();

  const CompactNodes &activeNodes = m_activeChart[startPos];
  size_t numNodes = activeNodes.GetSize();

  for (size_t i = 0; i < numNodes; ++i) {
	  const CompactNode &node = activeNodes.Get(i);
	  size_t nodeEndPos = node.m_endPos;
	  assert(nodeEndPos < endPos);

	  if (nodeEndPos + 1 == endPos) {
		  // 1-width span. lookup terminal
		  ExtendTerm(endPos);
	  }
	  ExtendNonTerm(nodeEndPos + 1, endPos);
  }

  ExtendNonTerm(startPos, endPos);
}

void ChartRuleLookupManagerCompactPT::ExtendTerm(size_t pos)
{

}

void ChartRuleLookupManagerCompactPT::ExtendNonTerm(size_t startPos, size_t endPos)
{

}

}  // namespace Moses
