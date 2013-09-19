/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "ChartCellLabelMBOT.h"
#include "ChartCellLabelSet.h"

#include <set>

namespace Moses
{

class ChartHypothesisCollection;

class ChartCellLabelSetMBOT : public ChartCellLabelSet
{
 private:
  typedef std::set<ChartCellLabelMBOT> SetTypeMBOT;

 public:
  typedef SetTypeMBOT::const_iterator const_iterator;
  ChartCellLabelSetMBOT(const WordsRange coverage) : ChartCellLabelSet(coverage)
  {
    m_mbotCoverage.push_back(coverage);
  }
  const_iterator begin() const { return m_mbotSet.begin(); }
  const_iterator end() const { return m_mbotSet.end(); }

  ~ChartCellLabelSetMBOT(){
  }

  //new for constructing
  void AddCoverage(WordsRange &coverage)
  {
      m_mbotCoverage.push_back(coverage);
  }

  void AddWordMBOT(const std::vector<Word> &w)
  {
    ChartCellLabelMBOT cellLabel(m_mbotCoverage.front(), w.front());
    std::vector<Word>::const_iterator itr_word;

    int counter = 1;
    for(itr_word = w.begin()+1; itr_word!=w.end();itr_word++)
    {
        cellLabel.AddLabel(*itr_word);
        if(counter < m_mbotCoverage.size())
        {cellLabel.AddCoverage(m_mbotCoverage[counter++]);}
    }
    m_mbotSet.insert(cellLabel);
  }


  void AddConstituent(const std::vector<Word> &w, const ChartHypothesisCollectionMBOT &stack)
  {

    std::vector<Word>::const_iterator itr_word;

    ChartCellLabelMBOT cellLabel = ChartCellLabelMBOT(m_mbotCoverage.front(), w.front(), &stack);

    int counter = 1;
    for(itr_word = w.begin()+1; itr_word!=w.end();itr_word++)
    {
        cellLabel.AddLabel(*itr_word);
        if(counter < m_mbotCoverage.size())
        {
            cellLabel.AddCoverage(m_mbotCoverage[counter++]);
        }
    }
    m_mbotSet.insert(cellLabel);
  }

  bool EmptyMBOT() const { return m_mbotSet.empty(); }

  size_t GetSizeMBOT() const { return m_mbotSet.size(); }


  const ChartCellLabelMBOT *FindMBOT(const std::vector<Word> &w) const
  {
    ChartCellLabelMBOT CellToFind = ChartCellLabelMBOT(m_mbotCoverage.front(), w.front());
     std::vector<Word>::const_iterator itr_word;

    int counter = 1;
    for(itr_word = w.begin()+1; itr_word!=w.end();itr_word++)
    {
        CellToFind.AddLabel(*itr_word);
        if(counter < m_mbotCoverage.size())
        {CellToFind.AddCoverage(m_mbotCoverage[counter++]);}
    }

    SetTypeMBOT::const_iterator p = m_mbotSet.find(CellToFind);
    return p == m_mbotSet.end() ? 0 : &(*p);
  }

 private:
  std::vector<WordsRange> m_mbotCoverage;
  SetTypeMBOT m_mbotSet;
};

}
