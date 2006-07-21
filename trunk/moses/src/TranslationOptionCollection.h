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

class DecodeStep;
class LanguageModel;
class FactorCollection;
class InputType;
class PhraseDictionary;

class TranslationOptionCollection : public std::list< TranslationOption >
{
	TranslationOptionCollection(const TranslationOptionCollection&); // no copy constructor
protected:
	InputType const& m_source;
	SquareMatrix m_futureScore;
	WordsBitmap m_initialCoverage;

	TranslationOptionCollection(InputType const& src);
	
public:
  virtual ~TranslationOptionCollection();

	void CreateTranslationOptions(const std::list < DecodeStep > &decodeStepList
																, const LMList &languageModels  														
																, const LMList &allLM
																, FactorCollection &factorCollection
																, float weightWordPenalty
																, bool dropUnknown
																, size_t verboseLevel);

	// get length/size of source input
	size_t GetSize() const;


	inline const SquareMatrix &GetFutureScore()
	{
		return m_futureScore;
	}
	inline const WordsBitmap &GetInitialCoverage() const 
	{
		return m_initialCoverage;
	}



 protected:
	virtual int HandleUnkownWord(PhraseDictionaryBase& phraseDictionary,
																size_t startPos,
																FactorCollection &factorCollection,
																const LMList &allLM,
																bool dropUnknown,
																float weightWordPenalty
																) =0; 
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

