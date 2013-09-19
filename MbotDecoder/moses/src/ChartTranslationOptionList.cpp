// $Id: ChartTranslationOptionList.cpp,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $
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
#include "StaticData.h"
#include "ChartTranslationOptionList.h"
#include "ChartTranslationOption.h"
#include "ChartTranslationOptionMBOT.h"
#include "ChartCellCollection.h"
#include "WordsRange.h"

namespace Moses
{

#ifdef USE_HYPO_POOL
ObjectPool<ChartTranslationOptionList> ChartTranslationOptionList::s_objectPool("ChartTranslationOptionList", 3000);
#endif

ChartTranslationOptionList::ChartTranslationOptionList(const WordsRange &range)
  :m_range(range)
{
  m_collection.reserve(200);
  m_scoreThreshold = std::numeric_limits<float>::infinity();
}

ChartTranslationOptionList::~ChartTranslationOptionList()
{
  RemoveAllInColl(m_collection);
}

class ChartTranslationOptionOrderer
{
public:
  bool operator()(const ChartTranslationOption* itemA, const ChartTranslationOption* itemB) const {
    //cast to mbot
    ChartTranslationOption * itemAnoConst = const_cast<ChartTranslationOption*>(itemA);
    ChartTranslationOptionMBOT * itemAmbot = static_cast<ChartTranslationOptionMBOT*>(itemAnoConst);

    ChartTranslationOption * itemBnoConst = const_cast<ChartTranslationOption*>(itemB);
    ChartTranslationOptionMBOT * itemBmbot = static_cast<ChartTranslationOptionMBOT*>(itemBnoConst);

    return itemAmbot->GetEstimateOfBestScoreMBOT() > itemBmbot->GetEstimateOfBestScoreMBOT();
  }
};

//FB : NEW : BEWARE : NEED TO BE ABLE TO PASS DOTTED RULE MBOT
void ChartTranslationOptionList::AddMBOT(const TargetPhraseCollection &targetPhraseCollection
                                     , const DottedRuleMBOT &dottedRule
                                     , const ChartCellCollection &chartCellColl
                                     , bool /* adhereTableLimit */
                                     , size_t ruleLimit)
{

  //std::cout << "ADDING MBOT TRANSLATION OPTION TO LIST" << std::endl;
  if (targetPhraseCollection.IsEmpty()) {
    //std::cout << "TARGET PHRASE COLLECTION IS EMPTY" << std::endl;
    return;
  }

//std::cout << "TARGET PHRASE COLLECTION NOT EMPTY" << std::endl;
  if (m_collection.size() < ruleLimit) {
    // not yet filled out quota. add everything
    //std::cout << "MAKING TRANSLATION OPTION" << std::endl;
    ChartTranslationOptionMBOT *option = new ChartTranslationOptionMBOT(
        targetPhraseCollection, dottedRule, m_range, chartCellColl);
   //std::cout << "TRANSLATION OPTION MADE" << dottedRule << std::endl;
    m_collection.push_back(option);
    float score = option->GetEstimateOfBestScoreMBOT();
    m_scoreThreshold = (score < m_scoreThreshold) ? score : m_scoreThreshold;
  }
  else {
    // full but not bursting. add if better than worst score
    ChartTranslationOptionMBOT option(targetPhraseCollection, dottedRule,
                                  m_range, chartCellColl);
    float score = option.GetEstimateOfBestScoreMBOT();
    //std::cout << "Obtained score : " << score << " : " << m_scoreThreshold << std::endl;
    if (score > m_scoreThreshold) {
      // dynamic allocation deferred until here on the assumption that most
      // options will score below the threshold.
      m_collection.push_back(new ChartTranslationOptionMBOT(option));
    }
  }

  // prune if bursting
  if (m_collection.size() > ruleLimit * 2) {
    std::nth_element(m_collection.begin()
                     , m_collection.begin() + ruleLimit
                     , m_collection.end()
                     , ChartTranslationOptionOrderer());
    // delete the bottom half
    for (size_t ind = ruleLimit; ind < m_collection.size(); ++ind) {

        ChartTranslationOption * itemNoConst = const_cast<ChartTranslationOption*>(m_collection[ind]);
        ChartTranslationOptionMBOT * itemMbot = static_cast<ChartTranslationOptionMBOT*>(itemNoConst);

      // make the best score of bottom half the score threshold
      float score = itemMbot->GetEstimateOfBestScoreMBOT();
      m_scoreThreshold = (score > m_scoreThreshold) ? score : m_scoreThreshold;
      delete m_collection[ind];
    }
    m_collection.resize(ruleLimit);
    //std::cout << "End of adding to Target Phrase Collection" << std::endl;
  }
}

void ChartTranslationOptionList::Add(const TargetPhraseCollection &targetPhraseCollection
                                     , const DottedRule &dottedRule
                                     , const ChartCellCollection &chartCellColl
                                     , bool /* adhereTableLimit */
                                     , size_t ruleLimit)
{

  //std::cout << "SHOULD NOT USE THIS ADDING METHOD" << std::endl;
  if (targetPhraseCollection.IsEmpty()) {
    return;
  }

  if (m_collection.size() < ruleLimit) {
    // not yet filled out quota. add everything
    ChartTranslationOption*option = new ChartTranslationOption(
        targetPhraseCollection, dottedRule, m_range, chartCellColl);
    m_collection.push_back(option);
    float score = option->GetEstimateOfBestScore();
    m_scoreThreshold = (score < m_scoreThreshold) ? score : m_scoreThreshold;
  }
  else {
    // full but not bursting. add if better than worst score
    ChartTranslationOption option(targetPhraseCollection, dottedRule,
                                  m_range, chartCellColl);
    float score = option.GetEstimateOfBestScore();
    if (score > m_scoreThreshold) {
      // dynamic allocation deferred until here on the assumption that most
      // options will score below the threshold.
      m_collection.push_back(new ChartTranslationOption(option));
    }
  }

  // prune if bursting
  if (m_collection.size() > ruleLimit * 2) {
    std::nth_element(m_collection.begin()
                     , m_collection.begin() + ruleLimit
                     , m_collection.end()
                     , ChartTranslationOptionOrderer());
    // delete the bottom half
    for (size_t ind = ruleLimit; ind < m_collection.size(); ++ind) {
      // make the best score of bottom half the score threshold
      float score = m_collection[ind]->GetEstimateOfBestScore();
      m_scoreThreshold = (score > m_scoreThreshold) ? score : m_scoreThreshold;
      delete m_collection[ind];
    }
    m_collection.resize(ruleLimit);
  }
}

void ChartTranslationOptionList::Add(ChartTranslationOption *transOpt)
{
  CHECK(transOpt);
  m_collection.push_back(transOpt);
}

//MAKE ADD MBOT TRANSLATION OPTION

void ChartTranslationOptionList::CreateChartRules(size_t ruleLimit)
{
  if (m_collection.size() > ruleLimit) {
    std::nth_element(m_collection.begin()
                     , m_collection.begin() + ruleLimit
                     , m_collection.end()
                     , ChartTranslationOptionOrderer());

    // delete the bottom half
    for (size_t ind = ruleLimit; ind < m_collection.size(); ++ind) {
      delete m_collection[ind];
    }
    m_collection.resize(ruleLimit);
  }
}


void ChartTranslationOptionList::Sort()
{
  // keep only those over best + threshold
  float scoreThreshold = -std::numeric_limits<float>::infinity();
  CollType::const_iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.end(); ++iter) {
    const ChartTranslationOption *transOpt = *iter;
    float score = transOpt->GetEstimateOfBestScore();
    scoreThreshold = (score > scoreThreshold) ? score : scoreThreshold;
  }

  scoreThreshold += StaticData::Instance().GetTranslationOptionThreshold();

  size_t ind = 0;
  while (ind < m_collection.size()) {
    const ChartTranslationOption *transOpt = m_collection[ind];
    if (transOpt->GetEstimateOfBestScore() < scoreThreshold) {
      std::cout << "SCORE LOWER THAN THRESHOLD!" << std::endl;
      delete transOpt;
      m_collection.erase(m_collection.begin() + ind);
    } else {
      ind++;
    }
  }

  std::sort(m_collection.begin(), m_collection.end(), ChartTranslationOptionOrderer());
}

void ChartTranslationOptionList::SortMBOT()
{
  // keep only those over best + threshold
  float scoreThreshold = -std::numeric_limits<float>::infinity();
  CollType::const_iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.end(); ++iter) {
    const ChartTranslationOption *constTransOpt = *iter;
    //remove const and cast to mbot
    ChartTranslationOption * transOpt = const_cast<ChartTranslationOption*>(constTransOpt);
    ChartTranslationOptionMBOT * mbotTransOpt = static_cast<ChartTranslationOptionMBOT*>(transOpt);

    float score = mbotTransOpt->GetEstimateOfBestScoreMBOT();
    //std::cout << "TRANS OPT : " << (*mbotTransOpt) << std::endl;
    //std::cout << "BEST ESTIMATE IN SORT : " << score << std::endl;
    scoreThreshold = (score > scoreThreshold) ? score : scoreThreshold;
  }

  scoreThreshold += StaticData::Instance().GetTranslationOptionThreshold();

  size_t ind = 0;
  while (ind < m_collection.size()) {
    const ChartTranslationOption *constTransOpt = m_collection[ind];
    //remove const and cast to mbot
    ChartTranslationOption * transOpt = const_cast<ChartTranslationOption*>(constTransOpt);
    ChartTranslationOptionMBOT * mbotTransOpt = static_cast<ChartTranslationOptionMBOT*>(transOpt);
    if (mbotTransOpt->GetEstimateOfBestScoreMBOT() < scoreThreshold) {
      delete transOpt;
      m_collection.erase(m_collection.begin() + ind);
    } else {
      ind++;
    }
  }

  std::sort(m_collection.begin(), m_collection.end(), ChartTranslationOptionOrderer());
}

}
