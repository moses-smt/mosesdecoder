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

#include "ChartCellLabel.h"

#include <set>

namespace Moses
{

class ChartHypothesisCollection;

class ChartCellLabelSet
{
 private:
  typedef std::set<ChartCellLabel> SetType;

 public:
  typedef SetType::const_iterator const_iterator;

  ChartCellLabelSet(const WordsRange &coverage) : m_coverage(coverage) {}

  const_iterator begin() const { return m_set.begin(); }
  const_iterator end() const { return m_set.end(); }

  void AddWord(const Word &w)
  {
    ChartCellLabel cellLabel(m_coverage, w);
    m_set.insert(cellLabel);
  }

  void AddConstituent(const Word &w, const ChartHypothesisCollection &stack)
  {
    ChartCellLabel cellLabel(m_coverage, w, &stack);
    m_set.insert(cellLabel);
  }

  bool Empty() const { return m_set.empty(); }

  size_t GetSize() const { return m_set.size(); }

  const ChartCellLabel *Find(const Word &w) const
  {
    SetType::const_iterator p = m_set.find(ChartCellLabel(m_coverage, w));
    return p == m_set.end() ? 0 : &(*p);
  }

 private:
  const WordsRange &m_coverage;
  SetType m_set;
};

}
