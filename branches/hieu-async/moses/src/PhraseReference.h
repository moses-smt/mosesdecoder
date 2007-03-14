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

#ifndef MOSES_PHRASE_REFERENCE_H
#define MOSES_PHRASE_REFERENCE_H

#include <iostream>
#include "InputType.h"
#include "WordsRange.h"

/***
 * hold a reference to a subphrase, the parent Phrase of which may be separately memory-managed
 */
class PhraseReference
{
	public:
	
		PhraseReference() : m_fullPhrase(NULL), m_range()
		{}
		PhraseReference(const InputType& phrase, const WordsRange& r) 
			: m_fullPhrase(&phrase), m_range(r) 
		{}
		const InputType& GetFullPhrase() const {return *m_fullPhrase;}
		Phrase GetSubphrase() const 
		{
			return m_fullPhrase->GetSubString(m_range);
		}
	
	protected:
	
		const InputType* m_fullPhrase;
		WordsRange m_range;
};

std::ostream& operator << (std::ostream& out, const PhraseReference& phrase);

#endif //MOSES_PHRASE_REFERENCE_H
