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

#include <vector>
#include <string>
#include "Word.h"
#include "Phrase.h"
#include "InputType.h"

class WordsRangs;
class PhraseDictionary;
class TranslationOptionCollection;


/***
 * A Phrase class with an ID. Used specifically as source input so contains functionality to read 
 *	from IODevice and create trans opt
 */
class Sentence : public Phrase, public InputType
{
 public:
	Sentence(FactorDirection direction)	: Phrase(direction), InputType()
	{
	}

	InputTypeEnum GetType() const
	{	return SentenceInput;}

	//! Calls Phrase::GetSubString(). Implements abstract InputType::GetSubString()
	Phrase GetSubString(const WordsRange& r) const 
	{
		return Phrase::GetSubString(r);
	}

	//! Calls Phrase::GetWord(). Implements abstract InputType::GetWord()
	const Word& GetWord(size_t pos) const
	{
		return Phrase::GetWord(pos);
	}

	//! Calls Phrase::GetSize(). Implements abstract InputType::GetSize()
	size_t GetSize() const 
	{
		return Phrase::GetSize();
	}

	int Read(std::istream& in,const std::vector<FactorType>& factorOrder);
	void Print(std::ostream& out) const;

	TranslationOptionCollection* CreateTranslationOptionCollection() const;
};

