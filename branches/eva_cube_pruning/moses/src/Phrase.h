// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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
#include <vector>
#include <list>
#include <string>
#include "Word.h"
#include "WordsBitmap.h"
#include "TypeDef.h"
#include "Util.h"
#include "mempool.h"

class Phrase
{
	friend std::ostream& operator<<(std::ostream&, const Phrase&);
 private:

	FactorDirection				m_direction;  /** Reusing Direction enum to really mean which language
																					Input = Source, Output = Target. 
																					Not really used, but nice to know for debugging purposes
																			*/
	size_t								m_phraseSize; //number of words
	size_t								m_arraySize;	/** current size of vector m_words. This number is equal or bigger
																					than m_phraseSize. Used for faster allocation of m_word */
	std::vector<Word>			m_words;

public:
	/** No longer does anything as not using mem pool for Phrase class anymore */
	static void InitializeMemPool();
	static void FinalizeMemPool();

	/** copy constructor */
	Phrase(const Phrase &copy);
	Phrase& operator=(const Phrase&);

	/** create empty phrase 
	* \param direction = language (Input = Source, Output = Target)
	*/
	Phrase(FactorDirection direction);
	/** create phrase from vectors of words	*/
	Phrase(FactorDirection direction, const std::vector< const Word* > &mergeWords);

	/** destructor */
	virtual ~Phrase();

	/** parse a string from phrase table or sentence input and create a 2D vector of strings
	*	\param phraseString string to parse
	*	\param factorOrder factors in the parse string. This argument is not fully used, only as a check to make ensure
	*											number of factors is what was promised
	*	\param factorDelimiter what char to use to separate factor strings from each other. Usually use '|'. Can be multi-char
	*/
	static std::vector< std::vector<std::string> > Parse(
																const std::string &phraseString
																, const std::vector<FactorType> &factorOrder
																, const std::string& factorDelimiter);
	/** Fills phrase with words from 2D string vector
		* \param factorOrder factor types of each element in 2D string vector
		* \param phraseVector 2D string vector
	*/
	void CreateFromString(const std::vector<FactorType> &factorOrder
              , const std::vector< std::vector<std::string> > &phraseVector);
	/** Fills phrase with words from format string, typically from phrase table or sentence input
		* \param factorOrder factor types of each element in 2D string vector
		* \param phraseString formatted input string to parse
		*	\param factorDelimiter delimiter, as used by Parse()
	*/
	void CreateFromString(const std::vector<FactorType> &factorOrder
											, const std::string &phraseString
											, const std::string &factorDelimiter);

	/**	copy factors from the other phrase to this phrase. 
		IsCompatible() must be run beforehand to ensure incompatible factors aren't overwritten
	*/
	void MergeFactors(const Phrase &copy);
	//! copy a single factor (specified by factorType)
	void MergeFactors(const Phrase &copy, FactorType factorType);
	//! copy all factors specified in factorVec and none others
	void MergeFactors(const Phrase &copy, const std::vector<FactorType>& factorVec);

	/** compare 2 phrases to ensure no factors are lost if the phrases are merged
	*	must run IsCompatible() to ensure incompatible factors aren't being overwritten
	*/
	bool IsCompatible(const Phrase &inputPhrase) const;
	bool IsCompatible(const Phrase &inputPhrase, FactorType factorType) const;
	bool IsCompatible(const Phrase &inputPhrase, const std::vector<FactorType>& factorVec) const;
	
	//! really means what language. Input = Source, Output = Target
	inline FactorDirection GetDirection() const
	{
		return m_direction;
	}

	//! number of words
	inline size_t GetSize() const
	{
		return m_phraseSize;
	}

	//! word at a particular position
	inline const Word &GetWord(size_t pos) const
	{
		return m_words[pos];
	}
	inline Word &GetWord(size_t pos)
	{
		return m_words[pos];
	}
	//! particular factor at a particular position
	inline const Factor *GetFactor(size_t pos, FactorType factorType) const
	{
		const Word &ptr = m_words[pos];
		return ptr[factorType];
	}
	inline void SetFactor(size_t pos, FactorType factorType, const Factor *factor)
	{
		Word &ptr = m_words[pos];
		ptr[factorType] = factor;
	}

	//! whether the 2D vector is a substring of this phrase
	bool Contains(const std::vector< std::vector<std::string> > &subPhraseVector
							, const std::vector<FactorType> &inputFactor) const;

	//! create an empty word at the end of the phrase
	Word &AddWord();
	//! create copy of input word at the end of the phrase
	void AddWord(const Word &newWord)
	{
    AddWord() = newWord;
  }
	//! create new phrase class that is a substring of this phrase
	Phrase GetSubString(const WordsRange &wordsRange) const;
	
	//! return a string rep of the phrase. Each factor is separated by the factor delimiter as specified in StaticData class
	std::string GetStringRep(const std::vector<FactorType> factorsToPrint) const; 
  
	TO_STRING();

	/** transitive comparison between 2 phrases
	*		used to insert & find phrase in dictionary
	*/
	bool operator< (const Phrase &compare) const;
	
	/** appends a phrase at the end of current phrase **/
	void Append(const Phrase &endPhrase);
	
	std::vector<Word> ReturnWords() const
	{
		return m_words;
	}
};

