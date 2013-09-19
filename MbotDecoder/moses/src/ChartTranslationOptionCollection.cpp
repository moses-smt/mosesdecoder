// $Id: ChartTranslationOptionCollection.cpp,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $
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
#include "DotChart.h"
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

  std::list<std::vector<DottedRule*>* >::iterator iterOuter;
  for (iterOuter = m_dottedRuleCache.begin(); iterOuter != m_dottedRuleCache.end(); ++iterOuter) {
    std::vector<DottedRule*> &inner = **iterOuter;
    RemoveAllInColl(inner);
  }
  RemoveAllInColl(m_dottedRuleCache);

  //FB : Also remove dotted rule cache
  std::list<std::vector<DottedRuleMBOT*>* >::iterator iterOuterMBOT;
    for (iterOuterMBOT = m_mbotDottedRuleCache.begin(); iterOuterMBOT != m_mbotDottedRuleCache.end(); ++iterOuterMBOT) {
      std::vector<DottedRuleMBOT*> &inner = **iterOuterMBOT;
      RemoveAllInColl(inner);
    }
    RemoveAllInColl(m_mbotDottedRuleCache);

}

void ChartTranslationOptionCollection::CreateTranslationOptionsForRange(
  size_t startPos
  , size_t endPos)
{

  //std::cout << "CTOC : CREATING TRANSLATION OPTIONS FOR RANGE ["<< startPos << ".." << endPos << "]"<< std::endl;
  ChartTranslationOptionList &chartRuleColl = GetTranslationOptionList(startPos, endPos);
  const WordsRange &wordsRange = chartRuleColl.GetSourceRange();

  CHECK(m_decodeGraphList.size() == m_ruleLookupManagers.size());
  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    const DecodeGraph &decodeGraph = **iterDecodeGraph;
    CHECK(decodeGraph.GetSize() == 1);
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t maxSpan = decodeGraph.GetMaxChartSpan();
    if (maxSpan == 0 || (endPos-startPos+1) <= maxSpan) {
      //std::cout << "CTOS" << wordsRange.GetStartPos() << "E" << wordsRange.GetEndPos() << std::endl;
      ruleLookupManager.GetChartRuleCollection(wordsRange, true, chartRuleColl);
    }
  }

  //MBOT specific
  ProcessUnknownWordMBOT(startPos, endPos);

  //beware : process unknown word
  //ProcessUnknownWord(startPos, endPos);

  //not implemented
  Prune(startPos, endPos);

  //use MBOT sort
  SortMBOT(startPos, endPos);

}

void ChartTranslationOptionCollection::ProcessUnknownWord(size_t sourcePos)
{
  const Word &sourceWord = m_source.GetWord(sourcePos);
  ProcessOneUnknownWord(sourceWord,sourcePos);

}

//! Force a creation of a translation option where there are none for a particular source position.
void ChartTranslationOptionCollection::ProcessUnknownWord(size_t startPos, size_t endPos)
{

  if (startPos != endPos) {
    // only for 1 word phrases
    return;
  }

  if (startPos == 0 || startPos == m_source.GetSize() - 1)
  { // don't create unknown words for <S> or </S> tags. Otherwise they can be moved. Should only be translated by glue rules
    return;
  }

  ChartTranslationOptionList &fullList = GetTranslationOptionList(startPos, startPos);
  const WordsRange &wordsRange = fullList.GetSourceRange();

  // try to translation for coverage with no trans by expanding table limit
  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    //const DecodeGraph &decodeGraph = **iterDecodeGraph;
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t numTransOpt = fullList.GetSize();
    if (numTransOpt == 0) {
      ruleLookupManager.GetChartRuleCollection(wordsRange, false, fullList);
    }
  }
  CHECK(iterRuleLookupManagers == m_ruleLookupManagers.end());

  bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
  // create unknown words for 1 word coverage where we don't have any trans options
  if (fullList.GetSize() == 0 || alwaysCreateDirectTranslationOption)
    ProcessUnknownWord(startPos);
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
    // create dotted rules
    std::vector<DottedRule*> *dottedRuleList = new std::vector<DottedRule*>();
    m_dottedRuleCache.push_back(dottedRuleList);
    dottedRuleList->push_back(new DottedRule());
    dottedRuleList->push_back(new DottedRule(sourceWordLabel, *(dottedRuleList->back())));

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
       //mbot : target phrase constructed using source phrase
      TargetPhrase *targetPhrase = new TargetPhrase(*m_unksrc);

      //Add Source Label to target phrase collection
      TargetPhraseCollection *tpc = new TargetPhraseCollection();
      tpc->Add(targetPhrase);

      m_cacheTargetPhraseCollection.push_back(tpc);
      Word &targetWord = targetPhrase->AddWord();
      targetWord.CreateUnknownWord(sourceWord);

      // scores
      vector<float> unknownScore(1, FloorScore(TransformScore(prob)));

      //targetPhrase->SetScore();
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetScore(m_system->GetWordPenaltyProducer(), wordPenaltyScore);
      //targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetTargetLHS(targetLHS);

      // chart rule
      ChartTranslationOption *chartRule = new ChartTranslationOption(*tpc
          , *dottedRuleList->back()
          , range
          , m_hypoStackColl);
      transOptColl.Add(chartRule);
    } // for (iterLHS
  } else {
    // drop source word. create blank trans opt
    vector<float> unknownScore(1, FloorScore(-numeric_limits<float>::infinity()));

    //mbot : target phrase constructed using source phrase
    TargetPhrase *targetPhrase = new TargetPhrase(*m_unksrc);
    TargetPhraseCollection *tpc = new TargetPhraseCollection();
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
      //targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetTargetLHS(targetLHS);

      // words consumed
      std::vector<DottedRule*> *dottedRuleList = new std::vector<DottedRule*>();
      m_dottedRuleCache.push_back(dottedRuleList);
      dottedRuleList->push_back(new DottedRule());
      dottedRuleList->push_back(new DottedRule(sourceWordLabel, *(dottedRuleList->back())));

      // chart rule
      ChartTranslationOption *chartRule = new ChartTranslationOption(*tpc
          , *dottedRuleList->back()
          , range
          , m_hypoStackColl);
      transOptColl.Add(chartRule);
    }
  }
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

void ChartTranslationOptionCollection::ProcessUnknownWordMBOT(size_t startPos, size_t endPos)
{
    //std::cout << "Process Unknown Word (1)" << std::endl;

  if (startPos != endPos) {
    // only for 1 word phrases
    return;
  }

  if (startPos == 0 || startPos == m_source.GetSize() - 1)
  { // don't create unknown words for <S> or </S> tags. Otherwise they can be moved. Should only be translated by glue rules
    return;
  }

  ChartTranslationOptionList &fullList = GetTranslationOptionList(startPos, startPos);
  const WordsRange &wordsRange = fullList.GetSourceRange();

  // try to translation for coverage with no trans by expanding table limit
  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    //const DecodeGraph &decodeGraph = **iterDecodeGraph;
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t numTransOpt = fullList.GetSize();
    if (numTransOpt == 0) {
      ruleLookupManager.GetChartRuleCollection(wordsRange, false, fullList);
    }
  }
  CHECK(iterRuleLookupManagers == m_ruleLookupManagers.end());

  bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
  // create unknown words for 1 word coverage where we don't have any trans options

  //MBOT : new : check that at least one POS tag corresponds to matches input tree otherwise consider as unknown word
  bool PosMatch = 0;
  for(size_t i= 0; i < fullList.GetSize();i++)
  {
	  for(size_t j=0; j < fullList.Get(i).GetTargetPhraseCollection().GetSize();j++)
	  {
		  const Word &sourceWord = m_source.GetWord(startPos);
		  vector<Word> sourceLabels;

		  //std::cout << "NON TERMINALS FOR UNKNOWN WORD : " << sourceWord << std::endl;
		  NonTerminalSet::const_iterator iter;
		  for(iter = m_source.GetLabelSet(startPos,endPos).begin();iter != m_source.GetLabelSet(startPos,endPos).end();iter++)
		  {
			  if(fullList.Get(i).GetTargetPhraseCollection().GetCollection()[j]->GetSourceLHS() == *iter)
			  {PosMatch = 1;}
		  }
	  }
  }
  if (fullList.GetSize() == 0 || PosMatch == 0 || alwaysCreateDirectTranslationOption)
	  ProcessUnknownWordMBOTWithSourceLabel(startPos,endPos);
}

// taken from ChartTranslationOptionCollectionText.
void ChartTranslationOptionCollection::ProcessUnknownWordMBOTWithSourceLabel(size_t startPos, size_t endPos)
{
  //std::cout << "Process Unknown Word (2)" << std::endl;
  const Word &sourceWord = m_source.GetWord(startPos);
  vector<Word> sourceLabels;

  //std::cout << "NON TERMINALS FOR UNKNOWN WORD : " << sourceWord << std::endl;
  NonTerminalSet::const_iterator iter;
  for(iter = m_source.GetLabelSet(startPos,endPos).begin();iter != m_source.GetLabelSet(startPos,endPos).end();iter++)
  {
	  //std::cout << "Pushed back to source : " << *iter << std::endl;
	  sourceLabels.push_back(*iter);
  }
  ProcessOneUnknownWordMBOT(sourceWord,startPos,sourceLabels);
}

//! special handling of ONE unknown words.
void ChartTranslationOptionCollection::ProcessOneUnknownWordMBOT(const Word &sourceWord, size_t sourcePos, vector<Word> sourceLabels, size_t)
{
    //std::cout << "Process Unknown Word (3)" << std::endl;
      // unknown word, add as trans opt
  const StaticData &staticData = StaticData::Instance();
  const UnknownWordPenaltyProducer *unknownWordPenaltyProducer = m_system->GetUnknownWordPenaltyProducer();
  vector<float> wordPenaltyScore(1, -0.434294482); // TODO what is this number?

  ChartTranslationOptionList &transOptColl = GetTranslationOptionList(sourcePos, sourcePos);
  const WordsRange &range = transOptColl.GetSourceRange();

  //std::cout << "Getting Chart Cell" << std::endl;
  ChartCell * chartCellptr = m_hypoStackColl.GetPtr(range);

  //convert into chartCellMBOT
  ChartCellMBOT * chartCell = static_cast<ChartCellMBOT*>(chartCellptr);

  //std::cout << "Getting Source Word Label" << std::endl;
  const ChartCellLabelMBOT &sourceWordLabel = chartCell->GetSourceWordLabel();

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

    //std::cout << "Creating dotted rules" << std::endl;
    // create dotted rules
    std::vector<DottedRuleMBOT*> *dottedRuleList = new std::vector<DottedRuleMBOT*>();
    m_mbotDottedRuleCache.push_back(dottedRuleList);
    dottedRuleList->push_back(new DottedRuleMBOT());
    dottedRuleList->push_back(new DottedRuleMBOT(sourceWordLabel, *(dottedRuleList->back())));

    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
		for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
			vector<Word>::iterator iter;
			for(iter = sourceLabels.begin();iter != sourceLabels.end();iter++)
			{
				const Word &sourceLabel = *iter;

			  const string &targetLHSStr = iterLHS->first;
			  //std::cout << "Getting Proba" << std::endl;
			  float prob = iterLHS->second;

			  // lhs
			  //const Word &sourceLHS = staticData.GetInputDefaultNonTerminal();
			  Word targetLHS(true);

			  targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
			  CHECK(targetLHS.GetFactor(0) != NULL);

			  // create MBOT lhs
			  vector<Word> mbotTargetLHS;
			  mbotTargetLHS.push_back(targetLHS);

			  // add to dictionary
			  TargetPhraseMBOT *targetPhrase = new TargetPhraseMBOT(*m_unksrc);
			  TargetPhraseCollection *tpc = new TargetPhraseCollection();
			  tpc->Add(targetPhrase);

			  m_cacheTargetPhraseCollection.push_back(tpc);
			  Word &targetWord = targetPhrase->AddWord();
			  targetWord.CreateUnknownWord(sourceWord);

			  // scores
			  vector<float> unknownScore(1, FloorScore(TransformScore(prob)));

			  //targetPhrase->SetScore();
			  targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
			  targetPhrase->SetScore(m_system->GetWordPenaltyProducer(), wordPenaltyScore);
			  targetPhrase->SetTargetLHSMBOT(mbotTargetLHS);
			  targetPhrase->SetSourceLHS(sourceLabel);

		  //std::cout << "Added word to TP : " << *targetPhrase << std::endl;

		  // chart rule
		  ChartTranslationOptionMBOT *chartRule = new ChartTranslationOptionMBOT(*tpc
			  , *dottedRuleList->back()
			  , range
			  , m_hypoStackColl);
		  transOptColl.Add(chartRule);
		}
	} // for (iterLHS
  } else {
    // drop source word. create blank trans opt
    vector<float> unknownScore(1, FloorScore(-numeric_limits<float>::infinity()));

    TargetPhrase *targetPhrase = new TargetPhraseMBOT(*m_unksrc);
    TargetPhraseCollection *tpc = new TargetPhraseCollection();
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
      //targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetTargetLHS(targetLHS);

      // words consumed
      std::vector<DottedRuleMBOT*> *dottedRuleList = new std::vector<DottedRuleMBOT*>();
      m_mbotDottedRuleCache.push_back(dottedRuleList);
      dottedRuleList->push_back(new DottedRuleMBOT());
      dottedRuleList->push_back(new DottedRuleMBOT(sourceWordLabel, *(dottedRuleList->back())));

      // chart rule
      //std::cout << "Making new chart rule" << std::endl;
      ChartTranslationOption *chartRule = new ChartTranslationOptionMBOT(*tpc
          , *dottedRuleList->back()
          , range
          , m_hypoStackColl);
      transOptColl.Add(chartRule);
    }
  }
}

void ChartTranslationOptionCollection::Add(ChartTranslationOption *transOpt, size_t pos)
{
  ChartTranslationOptionList &transOptList = GetTranslationOptionList(pos, pos);
  transOptList.Add(transOpt);
}

//! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
void ChartTranslationOptionCollection::Prune(size_t /* startPos */, size_t /* endPos */)
{

}

//! sort all trans opt in each list for cube pruning */
void ChartTranslationOptionCollection::Sort(size_t startPos, size_t endPos)
{
  ChartTranslationOptionList &list = GetTranslationOptionList(startPos, endPos);
  list.Sort();
}


void ChartTranslationOptionCollection::SortMBOT(size_t startPos, size_t endPos)
{
  ChartTranslationOptionList &list = GetTranslationOptionList(startPos, endPos);
  list.SortMBOT();
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

      for(int optionCounter = 0; optionCounter < list.GetSize(); optionCounter++)
      {
        std::cout << "Display for option counter : " << optionCounter << std::endl;
        const ChartTranslationOption * constOpt = list.GetPointer(optionCounter);
        ChartTranslationOption * opt = const_cast<ChartTranslationOption *> (constOpt);
        ChartTranslationOptionMBOT * mbotOpt = static_cast<ChartTranslationOptionMBOT *> (opt);
        out << (*mbotOpt) << endl;
      }
    }
  }
  return out;
}

}  // namespace
