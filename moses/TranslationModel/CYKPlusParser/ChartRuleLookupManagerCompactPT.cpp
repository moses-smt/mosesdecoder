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
#include "DotChartInMemory.h"

#include "moses/Util.h"
#include "moses/ChartParser.h"
#include "moses/InputType.h"
#include "moses/ChartParserCallback.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"
#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"

using namespace std;

namespace Moses
{

ChartRuleLookupManagerCompactPT::ChartRuleLookupManagerCompactPT(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellColl,
  const PhraseDictionaryCompact &pt)
  : ChartRuleLookupManager(parser, cellColl)
  , m_pt(pt)
{
  cerr << "starting ChartRuleLookupManagerCompactPT" << endl;
}

ChartRuleLookupManagerCompactPT::~ChartRuleLookupManagerCompactPT()
{
  RemoveAllInColl(m_tpColl);
}

void ChartRuleLookupManagerCompactPT::GetChartRuleCollection(
  const WordsRange &range,
  size_t lastPos,
  ChartParserCallback &outColl)
{
  size_t startPos = range.GetStartPos();
  size_t absEndPos = range.GetEndPos();

  m_lastPos = lastPos;
  m_stackVec.clear();
  m_outColl = &outColl;
  m_unaryPos = absEndPos-1; // rules ending in this position are unary and should not be added to collection

  const CompactPTNode &rootNode = m_root;



}

}  // namespace Moses
