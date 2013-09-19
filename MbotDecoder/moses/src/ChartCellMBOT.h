// $Id: ChartCellMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include "Word.h"
#include "WordsRange.h"
#include "NonTerminal.h"
#include "ChartHypothesisMBOT.h"
#include "ChartHypothesisCollectionMBOT.h"
#include "RuleCube.h"
#include "ChartCellLabelSetMBOT.h"
#include "ChartCell.h"

namespace Moses
{
class ChartTranslationOptionList;
class ChartTranslationOptionCollection;
class ChartCellCollection;
class ChartManager;

class ChartCellMBOT : public ChartCell
{
  friend std::ostream& operator<<(std::ostream&, const ChartCellMBOT&);
public:

protected:

  std::vector<WordsRange> m_mbotCoverage;
  ChartCellLabelMBOT *m_mbotSourceWordLabel;
  ChartCellLabelSetMBOT m_mbotTargetLabelSet;
  std::map<std::vector<Word>, ChartHypothesisCollectionMBOT> m_mbotHypoColl;

public:
  ChartCellMBOT(size_t startPos, size_t endPos, ChartManager &manager);
  ~ChartCellMBOT();

  void ProcessSentence(const ChartTranslationOptionList &transOptList
                       ,const ChartCellCollection &allChartCells);

  void ProcessSentenceWithSourceLabels(const ChartTranslationOptionList &transOptList
                          ,const ChartCellCollection &allChartCells, const InputType &source, size_t startPos, size_t endPos);

  // Get out target phrases that do not match source label
  void MarkPhrasesWithSourceLabels(std::vector<Word> sourceLabel);

  const HypoListMBOT &GetSortedHypothesesMBOT(const Word &constituentLabel) const;

  bool AddHypothesis(ChartHypothesisMBOT *hypo);

  void SortHypotheses();
  void PruneToSize();

  const ChartHypothesisMBOT *GetBestHypothesis() const;

  const ChartCellLabelMBOT &GetSourceWordLabel() const {
    CHECK(m_mbotCoverage.size()!=0);
    CHECK(m_mbotCoverage.front().GetNumWordsCovered() == 1);
    return *m_mbotSourceWordLabel;
  }

  const ChartCellLabelSetMBOT &GetTargetLabelSet() const {
    return m_mbotTargetLabelSet;
  }

  void CleanupArcList();

  void OutputSizes(std::ostream &out) const;
  size_t GetSize() const;

  //! transitive comparison used for adding objects into set
  inline bool operator<(const ChartCellMBOT &compare) const {
    return m_mbotCoverage < compare.m_mbotCoverage;
  }

  void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned,bool> &reachable) const;

};

}

