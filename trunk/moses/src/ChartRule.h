// $Id: ChartRule.h 3048 2010-04-05 17:25:26Z hieuhoang1972 $

#pragma once

#include <cassert>
#include <vector>
#include "Word.h"
#include "WordsRange.h"
#include "TargetPhrase.h"

namespace Moses
{
class WordConsumed;

// basically a phrase translation and the vector of words consumed to map each word
class ChartRule
{
	friend std::ostream& operator<<(std::ostream&, const ChartRule&);

protected:
	const Moses::TargetPhrase &m_targetPhrase;
	const WordConsumed &m_lastWordConsumed;
		/* map each source word in the phrase table to:
				1. a word in the input sentence, if the pt word is a terminal
				2. a 1+ phrase in the input sentence, if the pt word is a non-terminal
		*/
	std::vector<size_t> m_wordsConsumedTargetOrder;
		/* size is the size of the target phrase.
			Usually filled with NOT_KNOWN, unless the pos is a non-term, in which case its filled
			with its index 
		*/

	ChartRule(const ChartRule &copy); // not implmenented

public:
	ChartRule(const TargetPhrase &targetPhrase, const WordConsumed &lastWordConsumed)
	:m_targetPhrase(targetPhrase)
	,m_lastWordConsumed(lastWordConsumed)
	{}
	~ChartRule()
	{}

	const TargetPhrase &GetTargetPhrase() const
	{ return m_targetPhrase; }

	const WordConsumed &GetLastWordConsumed() const
	{ 
		return m_lastWordConsumed;
	}
	const std::vector<size_t> &GetWordsConsumedTargetOrder() const
	{	return m_wordsConsumedTargetOrder; }

	void CreateNonTermIndex();
};

}
