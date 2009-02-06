
#include <cassert>
#include "ChartTranslationOptionCollection.h"
#include "ChartTranslationOption.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/StaticData.h"
#include "../../moses/src/DecodeStep.h"
#include "../../moses/src/ChartRuleCollection.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

TranslationOptionCollection::TranslationOptionCollection(InputType const& source)
:m_source(source)
{
	// create 2-d vector
	size_t size = source.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		m_collection.push_back( vector< TranslationOptionList >() );

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
	RemoveAllInColl(m_cachedWordsConsumed);

}

void TranslationOptionCollection::CreateTranslationOptions(const std::vector <DecodeGraph*> &decodeGraphList)
{
	TRACE_ERR("Creating trans opt" << endl);
	size_t size = m_source.GetSize();

	std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
	for (iterDecodeGraph = decodeGraphList.begin(); iterDecodeGraph != decodeGraphList.end(); ++iterDecodeGraph)
	{
		const DecodeGraph &decodeGraph = **iterDecodeGraph;
		size_t maxChartSpan = decodeGraph.GetMaxChartSpan();

		for (size_t startPos = 0 ; startPos < size ; startPos++)
		{
			size_t maxEndPos = (maxChartSpan == 0) ? size : std::min(size, startPos + maxChartSpan);

			for (size_t endPos = startPos ; endPos < maxEndPos ; endPos++)
			{
				CreateTranslationOptionsForRange(decodeGraph, startPos, endPos, true);
			}
		}
	}

	ProcessUnknownWord(decodeGraphList);

	// Prune
	Prune();

	Sort();


}

void TranslationOptionCollection::CreateTranslationOptionsForRange(
																													 const DecodeGraph &decodeGraph
																													 , size_t startPos
																													 , size_t endPos
																													 , bool adhereTableLimit)
{
	assert(decodeGraph.GetSize() == 1);
	const DecodeStep &decodeStep = **decodeGraph.begin();

  const WordsRange wordsRange(startPos, endPos);

	TranslationOptionList &translationOptionList = GetTranslationOptionList(startPos, endPos);
	const PhraseDictionary &phraseDictionary = decodeStep.GetPhraseDictionary();

	const ChartRuleCollection *charTargetPhraseCollection = phraseDictionary.GetChartRuleCollection(m_source, wordsRange, adhereTableLimit);
	assert(charTargetPhraseCollection != NULL);

	ChartRuleCollection::const_iterator iterTargetPhrase;
	for (iterTargetPhrase = charTargetPhraseCollection->begin(); iterTargetPhrase != charTargetPhraseCollection->end(); ++iterTargetPhrase)
	{
		const ChartRule &rule = **iterTargetPhrase;
		TranslationOption *transOpt = new TranslationOption(wordsRange, rule, m_source);
		translationOptionList.Add(transOpt);
	}

	//delete charTargetPhraseCollection;
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

//! Force a creation of a translation option where there are none for a particular source position.
void TranslationOptionCollection::ProcessUnknownWord(const std::vector <Moses::DecodeGraph*> &decodeGraphList)
{
	size_t size = m_source.GetSize();
	// try to translation for coverage with no trans by expanding table limit
	for (size_t startVL = 0 ; startVL < decodeGraphList.size() ; startVL++)
	{
	  const DecodeGraph &decodeStepList = *decodeGraphList[startVL];
		for (size_t pos = 0 ; pos < size ; ++pos)
		{
				TranslationOptionList &fullList = GetTranslationOptionList(pos, pos);
				size_t numTransOpt = fullList.GetSize();
				if (numTransOpt == 0)
				{
					CreateTranslationOptionsForRange(decodeStepList
																				, pos, pos, false);
				}
		}
	}

	bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
	// create unknown words for 1 word coverage where we don't have any trans options
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		TranslationOptionList &fullList = GetTranslationOptionList(pos, pos);
		if (fullList.GetSize() == 0 || alwaysCreateDirectTranslationOption)
			ProcessUnknownWord(pos);
	}

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
	FactorCollection &factorCollection = FactorCollection::Instance();

	size_t isDigit = 0;
	if (StaticData::Instance().GetDropUnknown())
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
	Phrase* m_unksrc = new Phrase(Input); m_unksrc->AddWord() = sourceWord;
	m_unksrcs.push_back(m_unksrc);

	TranslationOption *transOpt;
	if (! StaticData::Instance().GetDropUnknown() || isDigit)
	{
		// add to dictionary
		TargetPhrase *targetPhrase = new TargetPhrase(Output);
		m_cacheTargetPhrase.push_back(targetPhrase);
		Word &targetWord = targetPhrase->AddWord();

		for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);

			const Factor *sourceFactor = sourceWord[currFactor];
			if (sourceFactor == NULL)
				targetWord[factorType] = factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR);
			else
				targetWord[factorType] = factorCollection.AddFactor(Output, factorType, sourceFactor->GetString());
		}

		targetPhrase->SetScore();
		targetPhrase->SetSourcePhrase(m_unksrc);
		//create a one-to-one aignment between UNKNOWN_FACTOR and its verbatim translation
		//TODO targetPhrase->CreateAlignmentInfo("(0)","(0)");

		// words consumed
		std::vector<WordsConsumed> *wordsConsumed = new std::vector<WordsConsumed>();
		m_cachedWordsConsumed.push_back(wordsConsumed);
		wordsConsumed->push_back(WordsConsumed(Moses::WordsRange(sourcePos, sourcePos), false));

		// chart rule
		ChartRule *chartRule = new ChartRule(*targetPhrase
																				, *wordsConsumed);
		m_cacheChartRule.push_back(chartRule);

		transOpt = new TranslationOption(Moses::WordsRange(sourcePos, sourcePos)
																	, *chartRule
																	, m_source);
	}
	else
	{ // drop source word. create blank trans opt
		TargetPhrase *targetPhrase = new TargetPhrase(Output);
		m_cacheTargetPhrase.push_back(targetPhrase);
		targetPhrase->SetSourcePhrase(m_unksrc);
		//TODO targetPhrase->SetAlignment();

		// words consumed
		std::vector<WordsConsumed> *wordsConsumed = new std::vector<WordsConsumed>;
		m_cachedWordsConsumed.push_back(wordsConsumed);
		wordsConsumed->push_back(WordsConsumed(Moses::WordsRange(sourcePos, sourcePos), false));

		// chart rule
		ChartRule *chartRule = new ChartRule(*targetPhrase
																				, *wordsConsumed);
		m_cacheChartRule.push_back(chartRule);

		transOpt = new TranslationOption(Moses::WordsRange(sourcePos, sourcePos)
															, *chartRule
															, m_source);
	}

	//transOpt->CalcScore();
	Add(transOpt, sourcePos);

}

void TranslationOptionCollection::Add(TranslationOption *transOpt, size_t pos)
{
	TranslationOptionList &transOptList = GetTranslationOptionList(pos, pos);
	transOptList.Add(transOpt);
}

//! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
void TranslationOptionCollection::Prune()
{

}

//! sort all trans opt in each list for cube pruning */
void TranslationOptionCollection::Sort()
{

}


}  // namespace


