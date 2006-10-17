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

#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Dictionary.h"
#include "TargetPhraseCollection.h"

class StaticData;
class InputType;
class WordsRange;

class PhraseDictionaryBase : public Dictionary, public ScoreProducer
{
 protected:
	size_t m_tableLimit;
	std::string m_filename;    // just for debugging purposes

 public:
	PhraseDictionaryBase(size_t noScoreComponent);
	virtual ~PhraseDictionaryBase();
		
	DecodeType GetDecodeType() const	{	return Translate;	}
	size_t GetTableLimit() const { return m_tableLimit; }
	
	virtual void InitializeForInput(InputType const&) {}
	const std::string GetScoreProducerDescription() const;
	unsigned int GetNumScoreComponents() const;

	virtual void SetWeightTransModel(const std::vector<float> &weightT)=0;

	virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const=0;
	virtual const TargetPhraseCollection *GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const;

	virtual void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)=0;
};
