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

/** abstract base class for phrase table classes
*/
class PhraseDictionary : public Dictionary, public ScoreProducer
{
 protected:
	size_t m_tableLimit;
	std::string m_filePath;    // just for debugging purposes

 public:
	PhraseDictionary(size_t numScoreComponent);
	virtual ~PhraseDictionary();
	
	DecodeType GetDecodeType() const	{	return Translate;	}
	//! table limit number. 
	size_t GetTableLimit() const { return m_tableLimit; }
	
	//! Overriden by load on demand phrase tables classes to load data for each input
	virtual void InitializeForInput(InputType const &/*source*/) {}
	const std::string GetScoreProducerDescription() const;
	size_t GetNumScoreComponents() const;

	/** set/change translation weights and recalc weighted score for each translation. 
		* TODO This may be redundant now we use ScoreCollection
	*/
	virtual void SetWeightTransModel(const std::vector<float> &weightT)=0;

	//! find list of translations that can translates src. Only for phrase input
	virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const=0;
	//! find list of translations that can translates a portion of src. Used by confusion network decoding
	virtual const TargetPhraseCollection *GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const;
	//! Create entry for translation of source to targetPhrase
	virtual void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)=0;
};
