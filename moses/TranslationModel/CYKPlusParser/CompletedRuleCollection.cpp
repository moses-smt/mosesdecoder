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

#include <iostream>
#include "CompletedRuleCollection.h"

#include "moses/StaticData.h"

using namespace std;

namespace Moses
{

CompletedRuleCollection::CompletedRuleCollection() : m_ruleLimit(StaticData::Instance().GetRuleLimit())
{
  m_scoreThreshold = numeric_limits<float>::infinity();
}

CompletedRuleCollection::~CompletedRuleCollection()
{
  Clear();
}

// copies some functionality (pruning) from ChartTranslationOptionList::Add
void CompletedRuleCollection::Add(const TargetPhraseCollection &tpc,
                                  const StackVec &stackVec,
                                  const ChartParserCallback &outColl)
{
  if (tpc.IsEmpty()) {
    return;
  }

  const TargetPhrase &targetPhrase = **(tpc.begin());
  float score = targetPhrase.GetFutureScore();
  for (StackVec::const_iterator p = stackVec.begin(); p != stackVec.end(); ++p) {
    float stackScore = (*p)->GetBestScore(&outColl);
    score += stackScore;
  }

  // If the rule limit has already been reached then don't add the option
  // unless it is better than at least one existing option.
  if (m_ruleLimit && m_collection.size() > m_ruleLimit && score < m_scoreThreshold) {
    return;
  }

  CompletedRule *completedRule = new CompletedRule(tpc, stackVec, score);
  m_collection.push_back(completedRule);

  // If the rule limit hasn't been exceeded then update the threshold.
  if (!m_ruleLimit || m_collection.size() <= m_ruleLimit) {
    m_scoreThreshold = (score < m_scoreThreshold) ? score : m_scoreThreshold;
  }

  // Prune if bursting
  if (m_ruleLimit && m_collection.size() == m_ruleLimit * 2) {
    NTH_ELEMENT4(m_collection.begin(),
                 m_collection.begin() + m_ruleLimit - 1,
                 m_collection.end(),
                 CompletedRuleOrdered());
    m_scoreThreshold = m_collection[m_ruleLimit-1]->GetScoreEstimate();
    for (size_t i = 0 + m_ruleLimit; i < m_collection.size(); i++) {
      delete m_collection[i];

    }
    m_collection.resize(m_ruleLimit);
  }
}


// copies some functionality (pruning) from ChartTranslationOptionList::Add
void CompletedRuleCollection::Add(const TargetPhraseCollection &tpc,
           const StackVec &stackVec,
           const std::vector<float> &stackScores,
           const ChartParserCallback &outColl)
{
    if (tpc.IsEmpty()) {
      return;
    }

    const TargetPhrase &targetPhrase = **(tpc.begin());
    float score = std::accumulate(stackScores.begin(), stackScores.end(), targetPhrase.GetFutureScore());

    // If the rule limit has already been reached then don't add the option
    // unless it is better than at least one existing option.
    if (m_collection.size() > m_ruleLimit && score < m_scoreThreshold) {
      return;
    }

    CompletedRule *completedRule = new CompletedRule(tpc, stackVec, score);
    m_collection.push_back(completedRule);

  // If the rule limit hasn't been exceeded then update the threshold.
  if (m_collection.size() <= m_ruleLimit) {
    m_scoreThreshold = (score < m_scoreThreshold) ? score : m_scoreThreshold;
  }

  // Prune if bursting
  if (m_collection.size() == m_ruleLimit * 2) {
        NTH_ELEMENT4(m_collection.begin(),
                     m_collection.begin() + m_ruleLimit - 1,
                     m_collection.end(),
                     CompletedRuleOrdered());
    m_scoreThreshold = m_collection[m_ruleLimit-1]->GetScoreEstimate();
    for (size_t i = 0 + m_ruleLimit; i < m_collection.size(); i++) {
      delete m_collection[i];

    }
    m_collection.resize(m_ruleLimit);
  }
}

}
