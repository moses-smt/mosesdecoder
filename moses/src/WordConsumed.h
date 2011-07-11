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
	const Word &m_mainWord; // can be target non-term, or source term
	const Word *m_otherWord; // can be source non-term, or NULL. Just for debug output

	const WordConsumed *m_prevWordsConsumed;
public:
	WordConsumed(); // not implmented
	WordConsumed(size_t startPos, size_t endPos, const Word &sourceWord, const Word *otherWord, const WordConsumed *prevWordsConsumed)
		:m_coverage(startPos, endPos)
		,m_mainWord(sourceWord)
		,m_otherWord(otherWord)
		,m_prevWordsConsumed(prevWordsConsumed)
	{}
	
	const Moses::WordsRange &GetWordsRange() const
	{ return m_coverage; }
	const Word &GetMainWord() const
	{	return m_mainWord; }
	const Word *GetOtherWord() const
	{	return m_otherWord; }
	Moses::WordsRange &GetWordsRange()
	{ return m_coverage; }
	bool IsNonTerminal() const
	{ return m_mainWord.IsNonTerminal(); }

	const WordConsumed *GetPrevWordsConsumed() const
	{ return m_prevWordsConsumed; }

	//! transitive comparison used for adding objects into FactorCollection
	inline bool operator<(const WordConsumed &compare) const
	{ 
		// TODO < on bool
		if (IsNonTerminal() < compare.IsNonTerminal())
			return true;
		else if (IsNonTerminal() == compare.IsNonTerminal())
			return m_coverage < compare.m_coverage; 

		return false;
	}

	int CompareWordsRange(const WordConsumed &compare) const;
};

}; // namespace
