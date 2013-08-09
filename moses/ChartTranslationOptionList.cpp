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
#include "WordsRange.h"
#include "InputType.h"
#include "InputPath.h"

using namespace std;

namespace Moses
{

ChartTranslationOptionList::ChartTranslationOptionList(size_t ruleLimit, const InputType &input)
  : m_size(0)
  , m_ruleLimit(ruleLimit)
{
  m_scoreThreshold = std::numeric_limits<float>::infinity();

  // create input paths
  size_t size = input.GetSize();
  m_inputPathMatrix.resize(size);
  for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;
      vector<InputPath*> &vec = m_inputPathMatrix[startPos];

      WordsRange range(startPos, endPos);
      Phrase subphrase(input.GetSubString(WordsRange(startPos, endPos)));
      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      InputPath *node;
      if (range.GetNumWordsCovered() == 1) {
        node = new InputPath(subphrase, labels, range, NULL, NULL);
        vec.push_back(node);
      } else {
        const InputPath &prevNode = GetInputPath(startPos, endPos - 1);
        node = new InputPath(subphrase, labels, range, &prevNode, NULL);
        vec.push_back(node);
      }

      //m_phraseDictionaryQueue.push_back(node);
    }
  }

}

ChartTranslationOptionList::~ChartTranslationOptionList()
{
  RemoveAllInColl(m_collection);

  InputPathMatrix::const_iterator iterOuter;
  for (iterOuter = m_inputPathMatrix.begin(); iterOuter != m_inputPathMatrix.end(); ++iterOuter) {
    const std::vector<InputPath*> &outer = *iterOuter;

    std::vector<InputPath*>::const_iterator iterInner;
    for (iterInner = outer.begin(); iterInner != outer.end(); ++iterInner) {
      InputPath *path = *iterInner;
      delete path;
    }
  }
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
                                     const WordsRange &range)
{
  if (tpc.IsEmpty()) {
    return;
  }

  float score = ChartTranslationOptions::CalcEstimateOfBestScore(tpc, stackVec);

  // If the rule limit has already been reached then don't add the option
  // unless it is better than at least one existing option.
  if (m_size > m_ruleLimit && score < m_scoreThreshold) {
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
  if (m_size <= m_ruleLimit) {
    m_scoreThreshold = (score < m_scoreThreshold) ? score : m_scoreThreshold;
  }

  // Prune if bursting
  if (m_size == m_ruleLimit * 2) {
    std::nth_element(m_collection.begin(),
                     m_collection.begin() + m_ruleLimit - 1,
                     m_collection.begin() + m_size,
                     ChartTranslationOptionOrderer());
    m_scoreThreshold = m_collection[m_ruleLimit-1]->GetEstimateOfBestScore();
    m_size = m_ruleLimit;
  }
}

void ChartTranslationOptionList::AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection*> &waste_memory, const WordsRange &range)
{
  TargetPhraseCollection *tpc = new TargetPhraseCollection();
  tpc->Add(&phrase);
  waste_memory.push_back(tpc);
  StackVec empty;
  Add(*tpc, empty, range);
}

void ChartTranslationOptionList::ApplyThreshold()
{
  if (m_size > m_ruleLimit) {
    // Something's gone wrong if the list has grown to m_ruleLimit * 2
    // without being pruned.
    assert(m_size < m_ruleLimit * 2);
    // Reduce the list to the best m_ruleLimit options.  The remaining
    // options can be overwritten on subsequent calls to Add().
    std::nth_element(m_collection.begin(),
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

  scoreThreshold += StaticData::Instance().GetTranslationOptionThreshold();

  CollType::iterator bound = std::partition(m_collection.begin(),
                             m_collection.begin()+m_size,
                             ScoreThresholdPred(scoreThreshold));

  m_size = std::distance(m_collection.begin(), bound);
}

InputPath &ChartTranslationOptionList::GetInputPath(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_inputPathMatrix[startPos].size());
  return *m_inputPathMatrix[startPos][offset];
}

}
