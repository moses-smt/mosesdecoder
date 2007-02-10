// $Id: InputType.h 886 2006-10-17 11:07:17Z hieuhoang1972 $
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

#include <string>
#include "TypeDef.h"
#include "Phrase.h"
#include "TargetPhraseCollection.h"

class WordsRange;
class Factor;
class PhraseDictionary;
class TranslationOptionCollection;

//! base class for sentences and confusion networks
class InputType 
{
protected:
	long m_translationId; 	//< contiguous Id
 
public:

	InputType(long translationId = 0);
	virtual ~InputType();

	long GetTranslationId() const
	{
		return m_translationId;
	}
	void SetTranslationId(long translationId)
	{
		m_translationId = translationId;
	}
	//! number of words in this sentence/confusion network
	virtual size_t GetSize() const =0;

	//! populate this InputType with data from in stream
	virtual int Read(std::istream& in,const std::vector<FactorType>& factorOrder) =0;
	
	//! Output debugging info to stream out
	virtual void Print(std::ostream&) const =0;

	//! create trans options specific to this InputType
	virtual TranslationOptionCollection* CreateTranslationOptionCollection() const=0;

	//! return substring. Only valid for Sentence class. TODO - get rid of this fn
	virtual Phrase GetSubString(const WordsRange&) const =0;

	//! return substring at a particular position. Only valid for Sentence class. TODO - get rid of this fn
	virtual const Word& GetWord(size_t pos) const=0;
	
	TO_STRING();
	
};

std::ostream& operator<<(std::ostream&,InputType const&);

