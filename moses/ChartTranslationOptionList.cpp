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

#include <algorithm>
#include <iostream>
#include <vector>
#include "StaticData.h"
#include "ChartTranslationOptionList.h"
#include "ChartTranslationOptions.h"
#include "ChartCellCollection.h"
#include "Range.h"
#include "InputType.h"
#include "InputPath.h"

using namespace std;

namespace Moses
{

ChartTranslationOptionList::
ChartTranslationOptionList(size_t ruleLimit, const InputType &input)
  : m_size(0)
  , m_ruleLimit(ruleLimit)
{
  m_scoreThreshold = std::numeric_limits<float>::infinity();
}

ChartTranslationOptionList::~ChartTranslationOptionList()
{
  RemoveAllInColl(m_collection);
}

void ChartTranslationOptionList::Clear()
{
  m_size = 0;
  m_scoreThreshold = std::numeric_limits<float>::infinity();
}

class ChartTranslationOptionOrderer
{
public:
  bool operator()(const ChartTranslationOptions* itemA, const ChartTranslationOptions* itemB) const {
    return itemA->GetEstimateOfBestScore() > itemB->GetEstimateOfBestScore();
  }
};

void ChartTranslationOptionList::Add(const TargetPhraseCollection &tpc,
                                     const StackVec &stackVec,
                                     const Range &range)
{
  if (tpc.IsEmpty()) {
    return;
  }

  for (size_t i = 0; i < stackVec.size(); ++i) {
    const ChartCellLabel &chartCellLabel = *stackVec[i];
    size_t numHypos = chartCellLabel.GetStack().cube->size();
    if (numHypos == 0) {
      return; // empty stack. These rules can't be used
    }
  }

  const TargetPhrase &targetPhrase = **(tpc.begin());
  float score = targetPhrase.GetFutureScore();
  for (StackVec::const_iterator p = stackVec.begin(); p != stackVec.end(); ++p) {
    score += (*p)->GetBestScore(this);
  }

  // If the rule limit has already been reached then don't add the option
  // unless it is better than at least one existing option.
  if (m_ruleLimit && m_size > m_ruleLimit && score < m_scoreThreshold) {
    return;
  }

  // Add the option to the list.
  if (m_size == m_collection.size()) {
    // m_collection has reached capacity: create a new object.
    m_collection.push_back(new ChartTranslationOptions(tpc, stackVec,
                           range, score));
  } else {
    // Overwrite an unused object.
    *(m_collection[m_size]) = ChartTranslationOptions(tpc, stackVec,
                              range, score);
  }
  ++m_size;

  // If the rule limit hasn't been exceeded then update the threshold.
  if (!m_ruleLimit || m_size <= m_ruleLimit) {
    m_scoreThreshold = (score < m_scoreThreshold) ? score : m_scoreThreshold;
  }

  // Prune if bursting
  if (m_ruleLimit && m_size == m_ruleLimit * 2) {
    NTH_ELEMENT4(m_collection.begin(),
                 m_collection.begin() + m_ruleLimit - 1,
                 m_collection.begin() + m_size,
                 ChartTranslationOptionOrderer());
    m_scoreThreshold = m_collection[m_ruleLimit-1]->GetEstimateOfBestScore();
    m_size = m_ruleLimit;
  }
}

void
ChartTranslationOptionList::
AddPhraseOOV(TargetPhrase &phrase,
             std::list<TargetPhraseCollection::shared_ptr > &waste_memory,
             const Range &range)
{
  TargetPhraseCollection::shared_ptr tpc(new TargetPhraseCollection);
  tpc->Add(&phrase);
  waste_memory.push_back(tpc);
  StackVec empty;
  Add(*tpc, empty, range);
}

void ChartTranslationOptionList::ApplyThreshold(float const threshold)
{
  if (m_ruleLimit && m_size > m_ruleLimit) {
    // Something's gone wrong if the list has grown to m_ruleLimit * 2
    // without being pruned.
    assert(m_size < m_ruleLimit * 2);
    // Reduce the list to the best m_ruleLimit options.  The remaining
    // options can be overwritten on subsequent calls to Add().
    NTH_ELEMENT4(m_collection.begin(),
                 m_collection.begin()+m_ruleLimit,
                 m_collection.begin()+m_size,
                 ChartTranslationOptionOrderer());
    m_size = m_ruleLimit;
  }

  // keep only those over best + threshold

  float scoreThreshold = -std::numeric_limits<float>::infinity();

  CollType::const_iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.begin()+m_size; ++iter) {
    const ChartTranslationOptions *transOpt = *iter;
    float score = transOpt->GetEstimateOfBestScore();
    scoreThreshold = (score > scoreThreshold) ? score : scoreThreshold;
  }

  scoreThreshold += threshold; // StaticData::Instance().GetTranslationOptionThreshold();

  CollType::iterator bound = std::partition(m_collection.begin(),
                             m_collection.begin()+m_size,
                             ScoreThresholdPred(scoreThreshold));

  m_size = std::distance(m_collection.begin(), bound);
}

float ChartTranslationOptionList::GetBestScore(const ChartCellLabel *chartCell) const
{
  const HypoList *stack = chartCell->GetStack().cube;
  assert(stack);
  assert(!stack->empty());
  const ChartHypothesis &bestHypo = **(stack->begin());
  return bestHypo.GetFutureScore();
}

void ChartTranslationOptionList::EvaluateWithSourceContext(const InputType &input, const InputPath &inputPath)
{
  // NEVER iterate over ALL of the collection. Just over the first m_size
  CollType::iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.begin() + m_size; ++iter) {
    ChartTranslationOptions &transOpts = **iter;
    transOpts.EvaluateWithSourceContext(input, inputPath);
  }

  // get rid of empty trans opts
  size_t numDiscard = 0;
  for (size_t i = 0; i < m_size; ++i) {
    ChartTranslationOptions *transOpts = m_collection[i];
    if (transOpts->GetSize() == 0) {
      //delete transOpts;
      ++numDiscard;
    } else if (numDiscard) {
      SwapTranslationOptions(i - numDiscard, i);
      //m_collection[] = transOpts;
    }
  }

  size_t newSize = m_size - numDiscard;
  m_size = newSize;
}

void ChartTranslationOptionList::SwapTranslationOptions(size_t a, size_t b)
{
  ChartTranslationOptions *transOptsA = m_collection[a];
  ChartTranslationOptions *transOptsB = m_collection[b];
  m_collection[a] = transOptsB;
  m_collection[b] = transOptsA;
}

std::ostream& operator<<(std::ostream &out, const ChartTranslationOptionList &obj)
{
  for (size_t i = 0; i < obj.m_collection.size(); ++i) {
    const ChartTranslationOptions &transOpts = *obj.m_collection[i];
    out << transOpts << endl;
  }
  return out;
}

}
