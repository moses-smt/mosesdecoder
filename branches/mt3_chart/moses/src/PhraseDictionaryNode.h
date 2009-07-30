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

#include <map>
#include <vector>
#include <iterator>
#include "Word.h"
#include "TargetPhraseCollection.h"

namespace Moses
{
class PhraseDictionary;

/** One node of the PhraseDictionaryMemory structure
*/
class PhraseDictionaryNode
{
	friend std::ostream& operator<<(std::ostream &out, const PhraseDictionaryNode &node);
	
public:
	virtual void CleanUp() = 0;
	virtual void Sort(size_t tableLimit) = 0;
	virtual const TargetPhraseCollection *GetTargetPhraseCollection() const = 0;
	virtual TargetPhraseCollection &GetOrCreateTargetPhraseCollection() = 0;
	virtual size_t GetSize() const = 0;

	// for mert
	virtual void SetWeightTransModel(const PhraseDictionary *phraseDictionary
													, const std::vector<float> &weightT) = 0;
	virtual const Word &GetSourceWord() const = 0;
	virtual void SetSourceWord(const Word &sourceWord) = 0;

	virtual std::string ToString() const = 0;
};

}
