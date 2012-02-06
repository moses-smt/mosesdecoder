// vim:tabstop=2
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

#include "util/check.hh"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "InputType.h"
#include "StaticData.h"
#include "DecodeStep.h"
#include "DummyScoreProducers.h"
#include "Util.h"

using namespace std;

namespace Moses
{

ChartTranslationOptionCollection::ChartTranslationOptionCollection(InputType const& source
    , const TranslationSystem* system
    , const ChartCellCollection &hypoStackColl
    , const std::vector<ChartRuleLookupManager*> &ruleLookupManagers)
  :m_source(source)
  ,m_system(system)
  ,m_decodeGraphList(system->GetDecodeGraphs())
  ,m_hypoStackColl(hypoStackColl)
  ,m_ruleLookupManagers(ruleLookupManagers)
  ,m_collection(source.GetSize())
{
  // create 2-d vector
  size_t size = source.GetSize();
  for (size_t startPos = 0 ; startPos < size ; ++startPos) {
    m_collection[startPos].reserve(size-startPos);
    for (size_t endPos = startPos ; endPos < size ; ++endPos) {
      m_collection[startPos].push_back( ChartTranslationOptionList(WordsRange(startPos, endPos)) );
    }
  }
}

ChartTranslationOptionCollection::~ChartTranslationOptionCollection()
{
  RemoveAllInColl(m_unksrcs);
  RemoveAllInColl(m_cacheTargetPhraseCollection);
}

void ChartTranslationOptionCollection::CreateTranslationOptionsForRange(
  size_t startPos
  , size_t endPos)
{
  ChartTranslationOptionList &chartRuleColl = GetTranslationOptionList(startPos, endPos);
  const WordsRange &wordsRange = chartRuleColl.GetSourceRange();

  assert(m_decodeGraphList.size() == m_ruleLookupManagers.size());
  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    const DecodeGraph &decodeGraph = **iterDecodeGraph;
    assert(decodeGraph.GetSize() == 1);
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t maxSpan = decodeGraph.GetMaxChartSpan();
    if (maxSpan == 0 || (endPos-startPos+1) <= maxSpan) {
      ruleLookupManager.GetChartRuleCollection(wordsRange, chartRuleColl);
    }
  }

  if (startPos == endPos && startPos != 0 && startPos != m_source.GetSize()-1) {
    bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
    if (chartRuleColl.GetSize() == 0 || alwaysCreateDirectTranslationOption) {
      // create unknown words for 1 word coverage where we don't have any trans options
      const Word &sourceWord = m_source.GetWord(startPos);
      ProcessOneUnknownWord(sourceWord, startPos);
    }
  }

  chartRuleColl.ApplyThreshold();
}

ChartTranslationOptionList &ChartTranslationOptionCollection::GetTranslationOptionList(size_t startPos, size_t endPos)
{
  size_t sizeVec = m_collection[startPos].size();
  CHECK(endPos-startPos < sizeVec);
  return m_collection[startPos][endPos - startPos];
}
const ChartTranslationOptionList &ChartTranslationOptionCollection::GetTranslationOptionList(size_t startPos, size_t endPos) const
{
  size_t sizeVec = m_collection[startPos].size();
  CHECK(endPos-startPos < sizeVec);
  return m_collection[startPos][endPos - startPos];
}

std::ostream& operator<<(std::ostream &out, const ChartTranslationOptionCollection &coll)
{
  std::vector< std::vector< ChartTranslationOptionList > >::const_iterator iterOuter;
  for (iterOuter = coll.m_collection.begin(); iterOuter != coll.m_collection.end(); ++iterOuter) {
    const std::vector< ChartTranslationOptionList > &vecInner = *iterOuter;
    std::vector< ChartTranslationOptionList >::const_iterator iterInner;

    for (iterInner = vecInner.begin(); iterInner != vecInner.end(); ++iterInner) {
      const ChartTranslationOptionList &list = *iterInner;
      out << list.GetSourceRange() << " = " << list.GetSize() << std::endl;
    }
  }


  return out;
}

//! special handling of ONE unknown words.
void ChartTranslationOptionCollection::ProcessOneUnknownWord(const Word &sourceWord, size_t sourcePos, size_t /* length */)
{
  // unknown word, add as trans opt
  const StaticData &staticData = StaticData::Instance();
  const UnknownWordPenaltyProducer *unknownWordPenaltyProducer = m_system->GetUnknownWordPenaltyProducer();
  vector<float> wordPenaltyScore(1, -0.434294482); // TODO what is this number?

  ChartTranslationOptionList &transOptColl = GetTranslationOptionList(sourcePos, sourcePos);
  const WordsRange &range = transOptColl.GetSourceRange();

  const ChartCell &chartCell = m_hypoStackColl.Get(range);
  const ChartCellLabel &sourceWordLabel = chartCell.GetSourceWordLabel();

  size_t isDigit = 0;
  if (staticData.GetDropUnknown()) {
    const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
    const string &s = f->GetString();
    isDigit = s.find_first_of("0123456789");
    if (isDigit == string::npos)
      isDigit = 0;
    else
      isDigit = 1;
    // modify the starting bitmap
  }

  Phrase* m_unksrc = new Phrase(1);
  m_unksrc->AddWord() = sourceWord;
  m_unksrcs.push_back(m_unksrc);

  //TranslationOption *transOpt;
  if (! staticData.GetDropUnknown() || isDigit) {
    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      float prob = iterLHS->second;

      // lhs
      //const Word &sourceLHS = staticData.GetInputDefaultNonTerminal();
      Word targetLHS(true);

      targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      CHECK(targetLHS.GetFactor(0) != NULL);

      // add to dictionary
      TargetPhrase *targetPhrase = new TargetPhrase(Output);
      TargetPhraseCollection *tpc = new TargetPhraseCollection;
      tpc->Add(targetPhrase);

      m_cacheTargetPhraseCollection.push_back(tpc);
      Word &targetWord = targetPhrase->AddWord();
      targetWord.CreateUnknownWord(sourceWord);

      // scores
      vector<float> unknownScore(1, FloorScore(TransformScore(prob)));

      //targetPhrase->SetScore();
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetScore(m_system->GetWordPenaltyProducer(), wordPenaltyScore);
      targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetTargetLHS(targetLHS);

      // chart rule
      ChartTranslationOption *chartRule = new ChartTranslationOption(*tpc
          , m_emptyStackVec
          , range);
      transOptColl.Add(chartRule);
    } // for (iterLHS
  } else {
    // drop source word. create blank trans opt
    vector<float> unknownScore(1, FloorScore(-numeric_limits<float>::infinity()));

    TargetPhrase *targetPhrase = new TargetPhrase(Output);
    TargetPhraseCollection *tpc = new TargetPhraseCollection;
    tpc->Add(targetPhrase);
    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      //float prob = iterLHS->second;

      Word targetLHS(true);
      targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      CHECK(targetLHS.GetFactor(0) != NULL);

      m_cacheTargetPhraseCollection.push_back(tpc);
      targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetTargetLHS(targetLHS);

      // chart rule
      ChartTranslationOption *chartRule = new ChartTranslationOption(*tpc
          , m_emptyStackVec
          , range);
      transOptColl.Add(chartRule);
    }
  }
}

}  // namespace
