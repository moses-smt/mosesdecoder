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
#include "ChartRuleLookupManagerOOVPT.h"
#include "DotChartInMemory.h"

#include "moses/Util.h"
#include "moses/ChartParser.h"
#include "moses/InputType.h"
#include "moses/ChartParserCallback.h"
#include "moses/StaticData.h"
#include "moses/NonTerminal.h"
#include "moses/ChartCellCollection.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"
#include "moses/TranslationModel/OOVPT.h"

using namespace std;

namespace Moses
{

ChartRuleLookupManagerOOVPT::ChartRuleLookupManagerOOVPT(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellColl,
  const OOVPT &oovPt)
  : ChartRuleLookupManager(parser, cellColl)
  , m_oovPt(oovPt)
{
  cerr << "starting ChartRuleLookupManagerOOVPT" << endl;
}

ChartRuleLookupManagerOOVPT::~ChartRuleLookupManagerOOVPT()
{
  RemoveAllInColl(m_tpColl);
}

void ChartRuleLookupManagerOOVPT::GetChartRuleCollection(
  const InputPath &inputPath,
  size_t last,
  ChartParserCallback &outColl)
{
  const WordsRange &range = inputPath.GetWordsRange();

  //m_tpColl.push_back(TargetPhraseCollection());
  //TargetPhraseCollection &tpColl = m_tpColl.back();
  TargetPhraseCollection *tpColl = new TargetPhraseCollection();
  m_tpColl.push_back(tpColl);

  if (range.GetNumWordsCovered() == 1) {
    const ChartCellLabel &sourceWordLabel = GetSourceAt(range.GetStartPos());
    const Word &sourceWord = sourceWordLabel.GetLabel();
    CreateTargetPhrases(sourceWord, *tpColl);
  }

  outColl.Add(*tpColl, m_stackVec, range);
}

void ChartRuleLookupManagerOOVPT::CreateTargetPhrases(const Word &sourceWord, TargetPhraseCollection &tpColl) const
{
  const StaticData &staticData = StaticData::Instance();
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      float prob = iterLHS->second;

      // lhs
      //const Word &sourceLHS = staticData.GetInputDefaultNonTerminal();
      Word *targetLHS = new Word(true);

      targetLHS->CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      UTIL_THROW_IF2(targetLHS->GetFactor(0) == NULL, "Null factor for target LHS");

      // add to dictionary
      TargetPhrase *targetPhrase = m_oovPt.CreateTargetPhrase(sourceWord);

      //targetPhrase->EvaluateInIsolation(*unksrc);
      targetPhrase->SetTargetLHS(targetLHS);
      if (staticData.IsDetailedTreeFragmentsTranslationReportingEnabled() || staticData.PrintNBestTrees() || staticData.GetTreeStructure() != NULL) {
        targetPhrase->SetProperty("Tree","[ " + (*targetLHS)[0]->GetString().as_string() + " "+sourceWord[0]->GetString().as_string()+" ]");
      }

      // chart rule
      tpColl.Add(targetPhrase);
    }
}
}  // namespace Moses
