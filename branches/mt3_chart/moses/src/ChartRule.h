
#pragma once

#include <vector>
#include "WordsRange.h"
#include "TargetPhrase.h"

namespace Moses
{

class WordsConsumed
{
protected:
	Moses::WordsRange	m_coverage;
	bool				m_isNonTerminal;
public:
	WordsConsumed()
		:m_coverage(NOT_FOUND, NOT_FOUND)
	{}

	WordsConsumed(Moses::WordsRange coverage, bool isNonTerminal)
		:m_coverage(coverage)
		,m_isNonTerminal(isNonTerminal)
	{}
	const Moses::WordsRange &GetWordsRange() const
	{ return m_coverage; }
	Moses::WordsRange &GetWordsRange()
	{ return m_coverage; }
	bool IsNonTerminal() const
	{ return m_isNonTerminal; }

	//! transitive comparison used for adding objects into FactorCollection
	inline bool operator<(const WordsConsumed &compare) const
	{ 
		if (m_isNonTerminal < compare.m_isNonTerminal)
			return true;
		else if (m_isNonTerminal == compare.m_isNonTerminal)
			return m_coverage < compare.m_coverage; 

		return false;
	}

};

// basically a phrase translation and the vector of words consumed to map each word
class ChartRule
{
	friend std::ostream& operator<<(std::ostream&, const ChartRule&);

protected:
	const Moses::TargetPhrase &m_targetPhrase;
	const std::vector<WordsConsumed> &m_wordsConsumed;
		/* map each source word in the phrase table to:
				1. a word in the input sentence, if the pt word is a terminal
				2. a 1+ phrase in the input sentence, if the pt word is a non-terminal
		*/
	std::vector<size_t> m_wordsConsumedTargetOrder;
		/* size is the size of the target phrase.
			Usually filled with NOT_KNOWN, unless the pos is a non-term, in which case its filled
			with its index 
		*/
public:
	ChartRule(const Moses::TargetPhrase &targetPhrase
					, const std::vector<WordsConsumed> &wordsConsumed);
	ChartRule(const ChartRule &copy)
		:m_targetPhrase(copy.m_targetPhrase)
		,m_wordsConsumed(copy.m_wordsConsumed)
	{}

	const Moses::TargetPhrase &GetTargetPhrase() const
	{ return m_targetPhrase; }
	const std::vector<WordsConsumed> &GetWordsConsumed() const
	{ return m_wordsConsumed; }
	const std::vector<size_t> GetWordsConsumedTargetOrder() const
	{	return m_wordsConsumedTargetOrder; }
};

}
