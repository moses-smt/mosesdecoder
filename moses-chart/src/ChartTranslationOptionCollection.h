
#pragma once

#include <vector>
#include "ChartTranslationOptionList.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/DecodeGraph.h"

namespace Moses
{
	class DecodeGraph;
	class Word;
	class ChartRule;
	class WordsConsumed;
};

namespace MosesChart
{

class TranslationOptionCollection
{
	friend std::ostream& operator<<(std::ostream&, const TranslationOptionCollection&);
protected:
	const Moses::InputType		&m_source;
	std::vector< std::vector< TranslationOptionList > >	m_collection; /*< contains translation options */
	std::vector<Moses::Phrase*> m_unksrcs;
	std::list<Moses::ChartRule*> m_cacheChartRule;
	std::list<Moses::TargetPhrase*> m_cacheTargetPhrase;
	std::list<std::vector<Moses::WordsConsumed>* > m_cachedWordsConsumed;

	virtual void CreateTranslationOptionsForRange(const Moses::DecodeGraph &decodeStepList
																			, size_t startPosition
																			, size_t endPosition
																			, bool adhereTableLimit);
	void Add(TranslationOptionList &translationOptionList);

	// for adding 1 trans opt in unknown word proc
	void Add(TranslationOption *transOpt, size_t pos);

	TranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos);
	const TranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos) const;

	//! Force a creation of a translation option where there are none for a particular source position.
	void ProcessUnknownWord(const std::vector <Moses::DecodeGraph*> &decodeGraphList);

	// taken from TranslationOptionCollectionText.
	void ProcessUnknownWord(size_t sourcePos);

	//! special handling of ONE unknown words.
	virtual void ProcessOneUnknownWord(const Moses::Word &sourceWord
																		 , size_t sourcePos, size_t length = 1);
	//! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
	void Prune();

	//! sort all trans opt in each list for cube pruning */
	void Sort();

public:
	TranslationOptionCollection(Moses::InputType const& source);
	virtual ~TranslationOptionCollection();
	virtual void CreateTranslationOptions(const std::vector <Moses::DecodeGraph*> &decodeGraphList);

	const TranslationOptionList &GetTranslationOptionList(const Moses::WordsRange &range) const
	{
		return GetTranslationOptionList(range.GetStartPos(), range.GetEndPos());
	}

};

}

