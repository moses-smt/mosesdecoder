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

#include "ChartCellLabel.h"
#include "NonTerminal.h"

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

namespace Moses
{

class ChartHypothesisCollection;

/** @todo I have no idea what's in here
 */
class ChartCellLabelSet
{
private:
#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_map<Word, ChartCellLabel,
          NonTerminalHasher, NonTerminalEqualityPred
          > MapType;
#else
  typedef std::map<Word, ChartCellLabel> MapType;
#endif

public:
  typedef MapType::const_iterator const_iterator;
  typedef MapType::iterator iterator;

  ChartCellLabelSet(const WordsRange &coverage) : m_coverage(coverage) {}

  const_iterator begin() const {
    return m_map.begin();
  }
  const_iterator end() const {
    return m_map.end();
  }

  iterator mutable_begin() {
    return m_map.begin();
  }
  iterator mutable_end() {
    return m_map.end();
  }

  void AddWord(const Word &w) {
    m_map.insert(std::make_pair(w, ChartCellLabel(m_coverage, w)));
  }

  // Stack is a HypoList or whatever the search algorithm uses.
  void AddConstituent(const Word &w, const HypoList *stack) {
    ChartCellLabel::Stack s;
    s.cube = stack;
    m_map.insert(std::make_pair(w, ChartCellLabel(m_coverage, w, s)));
  }

  bool Empty() const {
    return m_map.empty();
  }

  size_t GetSize() const {
    return m_map.size();
  }

  const ChartCellLabel *Find(const Word &w) const {
    MapType::const_iterator p = m_map.find(w);
    return p == m_map.end() ? 0 : &(p->second);
  }

  ChartCellLabel::Stack &FindOrInsert(const Word &w) {
    return m_map.insert(std::make_pair(w, ChartCellLabel(m_coverage, w))).first->second.MutableStack();
  }

private:
  const WordsRange &m_coverage;
  MapType m_map;
};

}
