// $Id$
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <cassert>
#include "PhraseDictionaryNodeSCFG.h"
#include "ChartTranslationOption.h"
#include "CoveredChartSpan.h"

namespace Moses
{

class DottedRule
{
  friend std::ostream& operator<<(std::ostream&, const DottedRule&);

protected:
  const PhraseDictionaryNodeSCFG &m_lastNode;
  const CoveredChartSpan *m_coveredChartSpan; // usually contains something, unless its the init processed rule
public:
  // used only to init dot stack.
  explicit DottedRule(const PhraseDictionaryNodeSCFG &lastNode)
    :m_lastNode(lastNode)
    ,m_coveredChartSpan(NULL)
  {}
  DottedRule(const PhraseDictionaryNodeSCFG &lastNode, const CoveredChartSpan *coveredChartSpan)
    :m_lastNode(lastNode)
    ,m_coveredChartSpan(coveredChartSpan)
  {}
  ~DottedRule() {
#ifdef USE_BOOST_POOL
    // Do nothing.  CoveredChartSpan objects are stored in object pools owned by
    // the sentence-specific ChartRuleLookupManagers.
#else
    delete m_coveredChartSpan;
#endif
  }
  const PhraseDictionaryNodeSCFG &GetLastNode() const {
    return m_lastNode;
  }
  const CoveredChartSpan *GetLastCoveredChartSpan() const {
    return m_coveredChartSpan;
  }
};

typedef std::vector<const DottedRule*> DottedRuleList;

// Collection of all DottedRules that share a common start point,
// grouped by end point.  Additionally, maintains a list of all
// DottedRules that could be expanded further, i.e. for which the
// corresponding PhraseDictionaryNodeSCFG is not a leaf.
class DottedRuleColl
{
protected:
  typedef std::vector<DottedRuleList> CollType;
  CollType m_coll;
  DottedRuleList m_expandableDottedRuleList;

public:
  typedef CollType::iterator iterator;
  typedef CollType::const_iterator const_iterator;

  const_iterator begin() const {
    return m_coll.begin();
  }
  const_iterator end() const {
    return m_coll.end();
  }
  iterator begin() {
    return m_coll.begin();
  }
  iterator end() {
    return m_coll.end();
  }

  DottedRuleColl(size_t size)
    : m_coll(size)
  {}

  ~DottedRuleColl();

  const DottedRuleList &Get(size_t pos) const {
    return m_coll[pos];
  }
  DottedRuleList &Get(size_t pos) {
    return m_coll[pos];
  }

  void Add(size_t pos, const DottedRule *dottedRule) {
    assert(dottedRule);
    m_coll[pos].push_back(dottedRule);
    if (!dottedRule->GetLastNode().IsLeaf()) {
      m_expandableDottedRuleList.push_back(dottedRule);
    }
  }

  void Clear(size_t pos) {
#ifdef USE_BOOST_POOL
    m_coll[pos].clear();
#endif
  }

  const DottedRuleList &GetExpandableDottedRuleList() const {
    return m_expandableDottedRuleList;
  }

};

}

