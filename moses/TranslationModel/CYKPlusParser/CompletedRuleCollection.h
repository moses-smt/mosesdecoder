/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2014 University of Edinburgh

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
#ifndef moses_CompletedRuleCollectionS_h
#define moses_CompletedRuleCollectionS_h

#include <vector>
#include <numeric>

#include "moses/StackVec.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/ChartTranslationOptions.h"
#include "moses/ChartCellLabel.h"
#include "moses/ChartParserCallback.h"

namespace Moses
{

// temporary storage for a completed rule (because we use lookahead to find rules before ChartManager wants us to)
struct CompletedRule {
public:

  CompletedRule(const TargetPhraseCollection &tpc,
                const StackVec &stackVec,
                const float score)
    : m_stackVec(stackVec)
    , m_tpc(tpc)
    , m_score(score) {}

  const TargetPhraseCollection & GetTPC() const {
    return m_tpc;
  }

  const StackVec & GetStackVector() const {
    return m_stackVec;
  }

  const float GetScoreEstimate() const {
    return m_score;
  }

private:
  const StackVec m_stackVec;
  const TargetPhraseCollection &m_tpc;
  const float m_score;

};

class CompletedRuleOrdered
{
public:
  bool operator()(const CompletedRule* itemA, const CompletedRule* itemB) const {
    return itemA->GetScoreEstimate() > itemB->GetScoreEstimate();
  }
};

struct CompletedRuleCollection {
public:

  CompletedRuleCollection();
  ~CompletedRuleCollection();

  CompletedRuleCollection(const CompletedRuleCollection &old)
    : m_collection(old.m_collection)
    , m_scoreThreshold(old.m_scoreThreshold)
    , m_ruleLimit(old.m_ruleLimit) {}

  CompletedRuleCollection & operator=(const CompletedRuleCollection &old) {

    m_collection = old.m_collection;
    m_scoreThreshold = old.m_scoreThreshold;
    m_ruleLimit = old.m_ruleLimit;
    return *this;
  }

  std::vector<CompletedRule*>::const_iterator begin() const {
    return m_collection.begin();
  }
  std::vector<CompletedRule*>::const_iterator end() const {
    return m_collection.end();
  }

  void Clear() {
    RemoveAllInColl(m_collection);
  }

  void Add(const TargetPhraseCollection &tpc,
           const StackVec &stackVec,
           const ChartParserCallback &outColl);

  void Add(const TargetPhraseCollection &tpc,
           const StackVec &stackVec,
           const std::vector<float> &stackScores,
           const ChartParserCallback &outColl);

private:
  std::vector<CompletedRule*> m_collection;
  float m_scoreThreshold;
  size_t m_ruleLimit;

};

} // namespace Moses

#endif