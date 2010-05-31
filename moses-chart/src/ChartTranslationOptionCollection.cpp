// $Id: PhraseDictionaryNewFormat.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $
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

#include <cassert>
#include "ChartTranslationOptionCollection.h"
#include "ChartTranslationOption.h"
#include "ChartCellCollection.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/StaticData.h"
#include "../../moses/src/DecodeStep.h"
#include "../../moses/src/ChartRuleCollection.h"
#include "../../moses/src/DummyScoreProducers.h"
#include "../../moses/src/WordConsumed.h"
#include "../../moses/src/Util.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

TranslationOptionCollection::TranslationOptionCollection(InputType const& source
																											, const std::vector<DecodeGraph*> &decodeGraphList
																											, const ChartCellCollection &hypoStackColl)
:m_source(source)
,m_decodeGraphList(decodeGraphList)
,m_hypoStackColl(hypoStackColl)
,m_collection(source.GetSize())
{
	// create 2-d vector
	size_t size = source.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		m_collection[startPos].reserve(size-startPos);
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
			m_collection[startPos].push_back( TranslationOptionList(WordsRange(startPos, endPos)) );
		}
	}
}

TranslationOptionCollection::~TranslationOptionCollection()
{
	RemoveAllInColl(m_unksrcs);
	RemoveAllInColl(m_cacheChartRule);
	RemoveAllInColl(m_cacheTargetPhrase);
	RemoveAllInColl(m_decodeGraphList);

	std::list<std::vector<Moses::WordConsumed*>* >::iterator iterOuter;
	for (iterOuter = m_cachedWordsConsumed.begin(); iterOuter != m_cachedWordsConsumed.end(); ++iterOuter)
	{
		std::vector<Moses::WordConsumed*> &inner = **iterOuter;
		RemoveAllInColl(inner);
	}

	RemoveAllInColl(m_cachedWordsConsumed);

}

void TranslationOptionCollection::CreateTranslationOptionsForRange(
																			size_t startPos
																		, size_t endPos)
{
	std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
	for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph)
	{
		const DecodeGraph &decodeGraph = **iterDecodeGraph;
		size_t maxSpan = decodeGraph.GetMaxChartSpan();
		if (maxSpan == 0 || (endPos-startPos+1) <= maxSpan)
		{
			CreateTranslationOptionsForRange(decodeGraph, startPos, endPos, true);
		}
	}

	ProcessUnknownWord(startPos, endPos);

	Prune(startPos, endPos);

	Sort(startPos, endPos);
	
}

//! Force a creation of a translation option where there are none for a particular source position.
void TranslationOptionCollection::ProcessUnknownWord(size_t startPos, size_t endPos)
{
	if (startPos != endPos)
	{ // only for 1 word phrases
		return;
	}

	TranslationOptionList &fullList = GetTranslationOptionList(startPos, startPos);

	// try to translation for coverage with no trans by expanding table limit
	std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
	for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph)
	{
		const DecodeGraph &decodeGraph = **iterDecodeGraph;
		size_t numTransOpt = fullList.GetSize();
		if (numTransOpt == 0)
		{
			CreateTranslationOptionsForRange(decodeGraph, startPos, startPos, false);
		}
	}

	bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
	// create unknown words for 1 word coverage where we don't have any trans options
	if (fullList.GetSize() == 0 || alwaysCreateDirectTranslationOption)
		ProcessUnknownWord(startPos);
}


void TranslationOptionCollection::CreateTranslationOptionsForRange(
																													 const DecodeGraph &decodeGraph
																													 , size_t startPos
																													 , size_t endPos
																													 , bool adhereTableLimit)
{
	assert(decodeGraph.GetSize() == 1);
	const DecodeStep &decodeStep = **decodeGraph.begin();

	// get wordsrange that doesn't go away until after sentence processing
	const WordsRange &wordsRange = GetTranslationOptionList(startPos, endPos).GetSourceRange();

	TranslationOptionList &translationOptionList = GetTranslationOptionList(startPos, endPos);
	const PhraseDictionary &phraseDictionary = decodeStep.GetPhraseDictionary();
	//cerr << phraseDictionary.GetScoreProducerDescription() << endl;
	
	const ChartRuleCollection *chartRuleCollection = phraseDictionary.GetChartRuleCollection(
																															m_source
																															, wordsRange
																															, adhereTableLimit
																															, m_hypoStackColl);
	assert(chartRuleCollection != NULL);
	//cerr << "chartRuleCollection size=" << chartRuleCollection->GetSize();
	
	translationOptionList.Reserve(translationOptionList.GetSize()+chartRuleCollection->GetSize());
	ChartRuleCollection::const_iterator iterTargetPhrase;
	for (iterTargetPhrase = chartRuleCollection->begin(); iterTargetPhrase != chartRuleCollection->end(); ++iterTargetPhrase)
	{
		const ChartRule &rule = **iterTargetPhrase;
		TranslationOption *transOpt = new TranslationOption(wordsRange, rule);
		translationOptionList.Add(transOpt);
	}
}

TranslationOptionList &TranslationOptionCollection::GetTranslationOptionList(size_t startPos, size_t endPos)
{
	size_t sizeVec = m_collection[startPos].size();
	assert(endPos-startPos < sizeVec);
	return m_collection[startPos][endPos - startPos];
}
const TranslationOptionList &TranslationOptionCollection::GetTranslationOptionList(size_t startPos, size_t endPos) const
{
	size_t sizeVec = m_collection[startPos].size();
	assert(endPos-startPos < sizeVec);
	return m_collection[startPos][endPos - startPos];
}

std::ostream& operator<<(std::ostream &out, const TranslationOptionCollection &coll)
{
  std::vector< std::vector< TranslationOptionList > >::const_iterator iterOuter;
  for (iterOuter = coll.m_collection.begin(); iterOuter != coll.m_collection.end(); ++iterOuter)
  {
	  const std::vector< TranslationOptionList > &vecInner = *iterOuter;
	  std::vector< TranslationOptionList >::const_iterator iterInner;

	  for (iterInner = vecInner.begin(); iterInner != vecInner.end(); ++iterInner)
	  {
		  const TranslationOptionList &list = *iterInner;
		  out << list.GetSourceRange() << " = " << list.GetSize() << std::endl;
	  }
  }


	return out;
}

// taken from TranslationOptionCollectionText.
void TranslationOptionCollection::ProcessUnknownWord(size_t sourcePos)
{
	const Word &sourceWord = m_source.GetWord(sourcePos);
	ProcessOneUnknownWord(sourceWord,sourcePos);
}

//! special handling of ONE unknown words.
void TranslationOptionCollection::ProcessOneUnknownWord(const Moses::Word &sourceWord
																	 , size_t sourcePos, size_t length)
{
	// unknown word, add as trans opt
	const StaticData &staticData = StaticData::Instance();
	const UnknownWordPenaltyProducer *unknownWordPenaltyProducer = staticData.GetUnknownWordPenaltyProducer();
	const WordPenaltyProducer *wordPenaltyProducer = staticData.GetWordPenaltyProducer();
	vector<float> wordPenaltyScore(1, -0.434294482);

	size_t isDigit = 0;
	if (staticData.GetDropUnknown())
	{
		const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
		const string &s = f->GetString();
		isDigit = s.find_first_of("0123456789");
		if (isDigit == string::npos)
			isDigit = 0;
		else
			isDigit = 1;
		// modify the starting bitmap
	}
	
	Phrase* m_unksrc = new Phrase(Input);
	m_unksrc->AddWord() = sourceWord;
	m_unksrcs.push_back(m_unksrc);

	TranslationOption *transOpt;
	if (! staticData.GetDropUnknown() || isDigit)
	{
		// words consumed
		std::vector<WordConsumed*> *wordsConsumed = new std::vector<WordConsumed*>();
		m_cachedWordsConsumed.push_back(wordsConsumed);
		
		WordConsumed *wc = new WordConsumed(sourcePos, sourcePos, sourceWord, NULL);
		wordsConsumed->push_back(wc);
		assert(wordsConsumed->size());

		// loop
		const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
		UnknownLHSList::const_iterator iterLHS; 
		for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS)
		{
			const string &targetLHSStr = iterLHS->first;
			float prob = iterLHS->second;
			
			// lhs
			const Word &sourceLHS = staticData.GetInputDefaultNonTerminal();
			Word targetLHS(true);

			targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
			assert(targetLHS.GetFactor(0) != NULL);

			// add to dictionary
			TargetPhrase *targetPhrase = new TargetPhrase(Output);

			m_cacheTargetPhrase.push_back(targetPhrase);
			Word &targetWord = targetPhrase->AddWord();
			targetWord.CreateUnknownWord(sourceWord);
			
			// scores
			vector<float> unknownScore(1, FloorScore(TransformScore(prob)));

			//targetPhrase->SetScore();
			targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
			targetPhrase->SetScore(wordPenaltyProducer, wordPenaltyScore);
			targetPhrase->SetSourcePhrase(m_unksrc);
			targetPhrase->SetTargetLHS(targetLHS);
						
			// chart rule
			ChartRule *chartRule = new ChartRule(*targetPhrase
																					, *wordsConsumed->back());
			chartRule->CreateNonTermIndex();
			m_cacheChartRule.push_back(chartRule);

			transOpt = new TranslationOption(wc->GetWordsRange(), *chartRule);
			//transOpt->CalcScore();
			Add(transOpt, sourcePos);
		} // for (iterLHS 
	}
	else
	{ // drop source word. create blank trans opt
		vector<float> unknownScore(1, FloorScore(-numeric_limits<float>::infinity()));

		TargetPhrase *targetPhrase = new TargetPhrase(Output);
		m_cacheTargetPhrase.push_back(targetPhrase);
		targetPhrase->SetSourcePhrase(m_unksrc);
		targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);

		// words consumed
		std::vector<WordConsumed*> *wordsConsumed = new std::vector<WordConsumed*>;
		m_cachedWordsConsumed.push_back(wordsConsumed);
		wordsConsumed->push_back(new WordConsumed(sourcePos, sourcePos, sourceWord, NULL));

		// chart rule
		assert(wordsConsumed->size());
		ChartRule *chartRule = new ChartRule(*targetPhrase
																				, *wordsConsumed->back());
		chartRule->CreateNonTermIndex();
		m_cacheChartRule.push_back(chartRule);

		transOpt = new TranslationOption(Moses::WordsRange(sourcePos, sourcePos)
															, *chartRule);
		//transOpt->CalcScore();
		Add(transOpt, sourcePos);
	}


}

void TranslationOptionCollection::Add(TranslationOption *transOpt, size_t pos)
{
	TranslationOptionList &transOptList = GetTranslationOptionList(pos, pos);
	transOptList.Add(transOpt);
}

//! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
void TranslationOptionCollection::Prune(size_t startPos, size_t endPos)
{

}

//! sort all trans opt in each list for cube pruning */
void TranslationOptionCollection::Sort(size_t startPos, size_t endPos)
{
	TranslationOptionList &list = GetTranslationOptionList(startPos, endPos);
	list.Sort();
}


}  // namespace


