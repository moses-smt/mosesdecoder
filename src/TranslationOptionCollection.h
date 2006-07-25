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
class InputType;
class PhraseDictionary;
class GenerationDictionary;
class InputType;
class LMList;

class TranslationOptionCollection : public std::list< TranslationOption >
{
	TranslationOptionCollection(const TranslationOptionCollection&); // no copy constructor
protected:
	InputType const													&m_source;
	SquareMatrix														m_futureScore;
	WordsBitmap															m_unknownWordPos;
	std::list<const PhraseDictionaryBase*>			m_allPhraseDictionary;
	std::list<const GenerationDictionary*>	m_allGenerationDictionary;
	std::set<TargetPhrase> m_unknownTargetPhrase;
	// make sure phrase doesn't go out of memory while we're using it
	const LMList *m_allLM;

	TranslationOptionCollection(InputType const& src);
	
	void CalcFutureScore(size_t verboseLevel);
															
public:
  virtual ~TranslationOptionCollection();

	// get length/size of source input
	size_t GetSize() const;

	virtual void CreateTranslationOptions(const std::list < DecodeStep > &decodeStepList
																			, const LMList &allLM
																			, FactorCollection &factorCollection
																			, float weightWordPenalty
																			, bool dropUnknown
																			, size_t verboseLevel);


	void Add(const TranslationOption &translationOption)
	{
		push_back(translationOption);
	}

	inline virtual const SquareMatrix &GetFutureScore() const
	{
		return m_futureScore;
	}



 protected:	
	virtual void ProcessInitialTranslation(const DecodeStep &decodeStep
															, FactorCollection &factorCollection
															, float weightWordPenalty
															, int dropUnknown
															, size_t verboseLevel
															, PartialTranslOptColl &outputPartialTranslOptColl)=0;

	virtual void ProcessUnknownWord(		size_t sourcePos
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty)=0;

	void ProcessGeneration(			const TranslationOption &inputPartialTranslOpt
															, const DecodeStep &decodeStep
															, PartialTranslOptColl &outputPartialTranslOptColl
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);
	void ProcessTranslation(		const TranslationOption &inputPartialTranslOpt
															, const DecodeStep &decodeStep
															, PartialTranslOptColl &outputPartialTranslOptColl
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);

	void ComputeFutureScores(size_t verboseLevel);
};

inline std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll)
{
	TranslationOptionCollection::const_iterator iter;
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		TRACE_ERR (*iter << std::endl);
	}	
	return out;
}

