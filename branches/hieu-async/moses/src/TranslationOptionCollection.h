// $Id$

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

#include <list>
#include "TypeDef.h"
#include "TranslationOption.h"
#include "SpanScore.h"
#include "WordsBitmap.h"
#include "PartialTranslOptColl.h"
#include "DecodeStep.h"

class LanguageModel;
class PhraseDictionaryMemory;
class GenerationDictionary;
class InputType;
class LMList;
class FactorMask;
class Word;

typedef std::vector<const TranslationOption*> TranslationOptionList;

/** Contains all phrase translations applicable to current input type (a sentence or confusion network).
 * A key insight into efficient decoding is that various input
 * conditions (trelliss, factored input, normal text, xml markup)
 * all lead to the same decoding algorithm: hypotheses are expanded
 * by applying phrase translations, which can be precomputed.
 *
 * The precomputation of a collection of instances of such TranslationOption 
 * depends on the input condition, but they all are presented to
 * decoding algorithm in the same form, using this class.
 *
 * This class cannot, and should not be instantiated directly. Instantiate 1 of the inherited
 * classes instead, for a particular input type
 **/

class TranslationOptionCollection
{
	friend std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll);
	TranslationOptionCollection(const TranslationOptionCollection&); /*< no copy constructor */
	typedef std::vector< std::vector< TranslationOptionList > > TransOptMatrix;
protected:
	InputType const			&m_source; /*< reference to the input */
	std::vector<TransOptMatrix>	m_collection; /*< contains translation options */
	SpanScore	m_futureScore; /*< matrix of future costs for contiguous parts (span) of the input */
	const size_t				m_maxNoTransOptPerCoverage; /*< maximum number of translation options per input span (phrase???) */
	
	TranslationOptionCollection(InputType const& src, size_t maxNoTransOptPerCoverage);
	
	void CalcFutureScore();

	//! Force a creation of a translation option where there are none for a particular source position.
	void ProcessUnknownWord();
	//! special handling of ONE unknown words.
	virtual void ProcessOneUnknownWord(size_t decodeStepId, const Word &sourceWord, size_t sourcePos);
	//! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
	void Prune();

	//! list of trans opt for a particular span
	TranslationOptionList &GetTranslationOptionList(
																		size_t decodeStepId
																		, size_t startPos
																		, size_t endPos)
	{
		return m_collection[decodeStepId][startPos][endPos - startPos];
	}

	//! list of trans opt for a particular span
	const TranslationOptionList &GetTranslationOptionList(
																		size_t decodeStepId
																		, size_t startPos
																		, size_t endPos) const
	{
		return m_collection[decodeStepId][startPos][endPos - startPos];
	}
	void Add(size_t decodeStepId, const TranslationOption *translationOption);

	//! implemented by inherited class, called by this class
	virtual void ProcessUnknownWord(size_t decodeStepId, size_t sourcePos)=0;

public:
  virtual ~TranslationOptionCollection();

	//! input sentence/confusion network
	const InputType& GetSource() const { return m_source; }

	//! get length/size of source input
	size_t GetSize() const;

	//! Create all possible translations from the phrase tables
	virtual void CreateTranslationOptions(const std::vector<const DecodeStep*> &decodeStepList);
	//! Create translation options that exactly cover a specific input span. 
	virtual void CreateTranslationOptionsForRange(const DecodeStep &decodeStep
																			, size_t startPosition
																			, size_t endPosition
																			, bool adhereTableLimit);

	//! list of trans opt for a particular span
	const TranslationOptionList &GetTranslationOptionList(const WordsRange &coverage) const
	{
		size_t decodeStepId = coverage.GetDecodeStepId();
		return GetTranslationOptionList(decodeStepId, coverage.GetStartPos(), coverage.GetEndPos());
	}

	//! score for untranslated parts of the sentence
	const SpanScore &GetFutureScoreObject() const
	{
		return m_futureScore;
	}

	TO_STRING();		
};

inline std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll)
{
	for (size_t decodeStepId = 0 ; decodeStepId < coll.m_collection.size() ; ++decodeStepId)
	{
		const TranslationOptionCollection::TransOptMatrix &matrix = coll.m_collection[decodeStepId];
		TranslationOptionCollection::TransOptMatrix::const_iterator iterStart;
		for (iterStart = matrix.begin(); iterStart != matrix.end(); ++iterStart)
		{
			const std::vector< TranslationOptionList > &startVec = *iterStart;
			out << "matrix size =" << startVec.size() << std::endl;
		}
	}
	return out;
}

