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
#include "SquareMatrix.h"
#include "WordsBitmap.h"
#include "PartialTranslOptColl.h"
#include "DecodeStep.h"

class LanguageModel;
class FactorCollection;
class PhraseDictionary;
class GenerationDictionary;
class InputType;
class LMList;
class FactorMask;

typedef std::vector<const TranslationOption*> TranslationOptionList;

/** Contains all phrase translations applicable to current sentence.
 * A key insight into efficient decoding is that various input
 * conditions (lattices, factored input, normal text, xml markup)
 * all lead to the same decoding algorithm: hypotheses are expanded
 * by applying phrase translations, which can be precomputed.
 *
 * The precomputation of a collection of instances of such TranslationOption 
 * depends on the input condition, but they all are presented to
 * decoding algorithm in the same form, using this class. **/

class TranslationOptionCollection
{
	friend std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll);
	TranslationOptionCollection(const TranslationOptionCollection&); /*< no copy constructor */
protected:
	std::vector< std::vector< TranslationOptionList > >	m_collection; /*< contains translation options */
	InputType const			&m_source;
	SquareMatrix				m_futureScore; /*< matrix of future costs for parts of the sentence */
	const size_t				m_maxNoTransOptPerCoverage; /*< maximum number of translation options per input span (phrase) */
	FactorCollection		*m_factorCollection;
	
	TranslationOptionCollection(InputType const& src, size_t maxNoTransOptPerCoverage);
	
	void CalcFutureScore();

	virtual void ProcessInitialTranslation(const DecodeStep &decodeStep
															, FactorCollection &factorCollection
															, PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos, size_t endPos, bool observeTableLimit );

	void ProcessUnknownWord(const std::list < DecodeStep* > &decodeStepList, FactorCollection &factorCollection);
	virtual void ProcessOneUnknownWord(const FactorArray &sourceWord
																		 , size_t sourcePos
																		 , FactorCollection &factorCollection);

	void ComputeFutureScores();	
	void Prune();

	TranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos)
	{
		return m_collection[startPos][endPos - startPos];
	}
	const TranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos) const
	{
	  return m_collection[startPos][endPos - startPos];
	}
	void Add(const TranslationOption *translationOption);
	
public:
  virtual ~TranslationOptionCollection();
	const InputType& GetSource() const { return m_source; }

	// get length/size of source input
	size_t GetSize() const;

	virtual void ProcessUnknownWord(size_t sourcePos
																	, FactorCollection &factorCollection)=0;

	virtual void CreateTranslationOptions(const std::list < DecodeStep* > &decodeStepList
																			, FactorCollection &factorCollection);

	virtual void CreateTranslationOptionsForRange(const std::list < DecodeStep* > &decodeStepList
																			, FactorCollection &factorCollection
																			, size_t startPosition
																			, size_t endPosition
																			, bool observeTableLimit);

	/** returns future cost matrix for sentence */
	inline virtual const SquareMatrix &GetFutureScore() const
	{
		return m_futureScore;
	}

	const TranslationOptionList &GetTranslationOptionList(const WordsRange &coverage) const
	{
		return GetTranslationOptionList(coverage.GetStartPos(), coverage.GetEndPos());
	}

	TO_STRING;		
};

inline std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll)
{
  std::vector< std::vector< TranslationOptionList > >::const_iterator i = coll.m_collection.begin();
	size_t j = 0;
	for (; i!=coll.m_collection.end(); ++i) {
    out << "s[" << j++ << "].size=" << i->size() << std::endl;
	}

	/*
	TranslationOptionCollection::const_iterator iter;
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		TRACE_ERR (*iter << std::endl);
	}	
	*/
	return out;
}

