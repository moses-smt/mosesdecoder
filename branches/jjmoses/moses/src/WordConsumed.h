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
