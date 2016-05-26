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
#include "moses/FactorCollection.h"

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

  typedef std::vector<ChartCellLabel*> MapType;

public:
  typedef MapType::const_iterator const_iterator;
  typedef MapType::iterator iterator;

  ChartCellLabelSet(const Range &coverage)
    : m_coverage(coverage)
    , m_map(FactorCollection::Instance().GetNumNonTerminals(), NULL)
    , m_size(0) { }

  ~ChartCellLabelSet() {
    RemoveAllInColl(m_map);
  }

  // TODO: skip empty elements when iterating, or deprecate this
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
    size_t idx = w[0]->GetId();
    if (! ChartCellExists(idx)) {
      m_size++;


      m_map[idx] = new ChartCellLabel(m_coverage, w);
    }
  }

  // Stack is a HypoList or whatever the search algorithm uses.
  void AddConstituent(const Word &w, const HypoList *stack) {
    size_t idx = w[0]->GetId();
    if (ChartCellExists(idx)) {
      ChartCellLabel::Stack & s = m_map[idx]->MutableStack();
      s.cube = stack;
    } else {
      ChartCellLabel::Stack s;
      s.cube = stack;
      m_size++;
      m_map[idx] = new ChartCellLabel(m_coverage, w, s);
    }
  }

  // grow vector if necessary
  bool ChartCellExists(size_t idx) {
    try {
      if (m_map.at(idx) != NULL) {
        return true;
      }
    } catch (const std::out_of_range& oor) {
      m_map.resize(FactorCollection::Instance().GetNumNonTerminals(), NULL);
    }
    return false;
  }

  bool Empty() const {
    return m_size == 0;
  }

  size_t GetSize() const {
    return m_size;
  }

  const ChartCellLabel *Find(const Word &w) const {
    size_t idx = w[0]->GetId();
    try {
      return m_map.at(idx);
    } catch (const std::out_of_range& oor) {
      return NULL;
    }
  }

  const ChartCellLabel *Find(size_t idx) const {
    try {
      return m_map.at(idx);
    } catch (const std::out_of_range& oor) {
      return NULL;
    }
  }

  ChartCellLabel::Stack &FindOrInsert(const Word &w) {
    size_t idx = w[0]->GetId();
    if (! ChartCellExists(idx)) {
      m_size++;
      m_map[idx] = new ChartCellLabel(m_coverage, w);
    }
    return m_map[idx]->MutableStack();
  }

private:
  const Range &m_coverage;
  MapType m_map;
  size_t m_size;
};

}
