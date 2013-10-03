/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2011 University of Edinburgh

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
#include "ChartRuleLookupManagerSkeleton.h"
#include "DotChartInMemory.h"

#include "moses/Util.h"
#include "moses/ChartParser.h"
#include "moses/InputType.h"
#include "moses/ChartParserCallback.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"
#include "moses/TranslationModel/SkeletonPT.h"

using namespace std;

namespace Moses
{

ChartRuleLookupManagerSkeleton::ChartRuleLookupManagerSkeleton(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellColl,
  const SkeletonPT &skeletonPt)
  : ChartRuleLookupManager(parser, cellColl)
  , m_skeletonPT(skeletonPt)
{
  cerr << "starting ChartRuleLookupManagerSkeleton" << endl;
}

ChartRuleLookupManagerSkeleton::~ChartRuleLookupManagerSkeleton()
{
  RemoveAllInColl(m_tpColl);
}

void ChartRuleLookupManagerSkeleton::GetChartRuleCollection(
  const WordsRange &range,
  ChartParserCallback &outColl)
{
  //m_tpColl.push_back(TargetPhraseCollection());
  //TargetPhraseCollection &tpColl = m_tpColl.back();
  TargetPhraseCollection *tpColl = new TargetPhraseCollection();
  m_tpColl.push_back(tpColl);

  if (range.GetNumWordsCovered() == 1) {
    const ChartCellLabel &sourceWordLabel = GetSourceAt(range.GetStartPos());
    const Word &sourceWord = sourceWordLabel.GetLabel();
    TargetPhrase *tp = CreateTargetPhrase(sourceWord);
    tpColl->Add(tp);
  }

  outColl.Add(*tpColl, m_stackVec, range);
}

TargetPhrase *ChartRuleLookupManagerSkeleton::CreateTargetPhrase(const Word &sourceWord) const
{
  // create a target phrase from the 1st word of the source, prefix with 'ChartManagerSkeleton:'
  string str = sourceWord.GetFactor(0)->GetString().as_string();
  str = "ChartManagerSkeleton:" + str;

  TargetPhrase *tp = new TargetPhrase();
  Word &word = tp->AddWord();
  word.CreateFromString(Output, m_skeletonPT.GetOutput(), str, false);

  // create hiero-style non-terminal for LHS
  Word *targetLHS = new Word();
  targetLHS->CreateFromString(Output, m_skeletonPT.GetOutput(), "X", true);
  tp->SetTargetLHS(targetLHS);

  // word alignement
  tp->SetAlignmentInfo("0-0");

  // score for this phrase table
  vector<float> scores(m_skeletonPT.GetNumScoreComponents(), 1.3);
  tp->GetScoreBreakdown().PlusEquals(&m_skeletonPT, scores);

  // score of all other ff when this rule is being loaded
  Phrase sourcePhrase;
  sourcePhrase.AddWord(sourceWord);
  //tp->Evaluate(sourcePhrase, m_skeletonPT.GetFeaturesToApply());

  return tp;
}
}  // namespace Moses
