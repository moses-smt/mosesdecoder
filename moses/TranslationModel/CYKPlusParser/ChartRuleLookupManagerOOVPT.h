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

#include <vector>
#include "moses/ChartRuleLookupManager.h"
#include "moses/StackVec.h"

namespace Moses
{
class TargetPhraseCollection;
class ChartParserCallback;
class DottedRuleColl;
class WordsRange;
class OOVPT;

class ChartRuleLookupManagerOOVPT : public ChartRuleLookupManager
{
public:
  ChartRuleLookupManagerOOVPT(const ChartParser &parser,
                                 const ChartCellCollectionBase &cellColl,
                                 const OOVPT &oovPt);

  ~ChartRuleLookupManagerOOVPT();

  virtual void GetChartRuleCollection(
    const InputPath &inputPath,
    size_t last,
    ChartParserCallback &outColl);

private:
  void CreateTargetPhrases(const Word &sourceWord, TargetPhraseCollection &tpColl) const;

  StackVec m_stackVec;
  std::vector<TargetPhraseCollection*> m_tpColl;
  const OOVPT &m_oovPt;
};

}  // namespace Moses

