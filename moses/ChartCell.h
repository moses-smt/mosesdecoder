// $Id$
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
#include "ChartHypothesis.h"
#include "ChartHypothesisCollection.h"
#include "RuleCube.h"
#include "ChartCellLabelSet.h"

#include <boost/scoped_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

namespace Moses
{
class ChartTranslationOptionList;
class ChartCellCollection;
class ChartManager;

class ChartCellBase
{
public:
  ChartCellBase(size_t startPos, size_t endPos);

  virtual ~ChartCellBase();

  const ChartCellLabelSet &GetTargetLabelSet() const {
    return m_targetLabelSet;
  }

  ChartCellLabelSet &MutableTargetLabelSet() {
    return m_targetLabelSet;
  }

  const WordsRange &GetCoverage() const {
    return m_coverage;
  }

protected:
  const WordsRange m_coverage;
  ChartCellLabelSet m_targetLabelSet;
};

/** 1 cell in chart decoder.
 *  Doesn't directly hold hypotheses. Each cell contain a map of ChartHypothesisCollection that have different constituent labels
 */
class ChartCell : public ChartCellBase
{
  friend std::ostream& operator<<(std::ostream&, const ChartCell&);
public:
#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_map<Word,
          ChartHypothesisCollection,
          NonTerminalHasher,
          NonTerminalEqualityPred
          > MapType;
#else
  typedef std::map<Word, ChartHypothesisCollection> MapType;
#endif

protected:
  MapType m_hypoColl;

  bool m_nBestIsEnabled; /**< flag to determine whether to keep track of old arcs */
  ChartManager &m_manager;

public:
  ChartCell(size_t startPos, size_t endPos, ChartManager &manager);
  ~ChartCell();

  void ProcessSentence(const ChartTranslationOptionList &transOptList
                       ,const ChartCellCollection &allChartCells);

  //! Get all hypotheses in the cell that have the specified constituent label
  const HypoList *GetSortedHypotheses(const Word &constituentLabel) const {
    MapType::const_iterator p = m_hypoColl.find(constituentLabel);
    return (p == m_hypoColl.end()) ? NULL : &(p->second.GetSortedHypotheses());
  }

  //! for n-best list
  const HypoList *GetAllSortedHypotheses() const;

  bool AddHypothesis(ChartHypothesis *hypo);

  void SortHypotheses();
  void PruneToSize();

  const ChartHypothesis *GetBestHypothesis() const;

  void CleanupArcList();

  void OutputSizes(std::ostream &out) const;
  size_t GetSize() const;

  //! transitive comparison used for adding objects into set
  inline bool operator<(const ChartCell &compare) const {
    return m_coverage < compare.m_coverage;
  }

  void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned,bool> &reachable) const;

};

}

