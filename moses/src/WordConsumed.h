#pragma once

#include <iostream>
#include "WordsRange.h"
#include "Word.h"

namespace Moses
{

class WordConsumed
{
	friend std::ostream& operator<<(std::ostream&, const WordConsumed&);

protected:
	WordsRange	m_coverage;
	const Word &m_sourceWord; // can be non-term headword, or term
	const WordConsumed *m_prevWordsConsumed;
public:
	WordConsumed(); // not implmented
	WordConsumed(size_t startPos, size_t endPos, const Word &sourceWord, const WordConsumed *prevWordsConsumed)
		:m_coverage(startPos, endPos)
		,m_sourceWord(sourceWord)
		,m_prevWordsConsumed(prevWordsConsumed)
	{}
	const Moses::WordsRange &GetWordsRange() const
	{ return m_coverage; }
	const Word &GetSourceWord() const
	{
		return m_sourceWord; 
	}
	Moses::WordsRange &GetWordsRange()
	{ return m_coverage; }
	bool IsNonTerminal() const
	{ return m_sourceWord.IsNonTerminal(); }

	const WordConsumed *GetPrevWordsConsumed() const
	{ return m_prevWordsConsumed; }

	//! transitive comparison used for adding objects into FactorCollection
	inline bool operator<(const WordConsumed &compare) const
	{ 
		if (IsNonTerminal() < compare.IsNonTerminal())
			return true;
		else if (IsNonTerminal() == compare.IsNonTerminal())
			return m_coverage < compare.m_coverage; 

		return false;
	}

};

}; // namespace
