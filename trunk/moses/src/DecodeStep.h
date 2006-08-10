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

#include <cassert>
#include "TypeDef.h"
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"

class Dictionary;

/** Specification for a decoding step.
 * The factored translation model consists of Translation and Generation
 * steps, which consult a Dictionary of phrase translations or word
 * generations. This class implements the specification for one of these
 * steps, both the DecodeType and a pointer to the Dictionary
 **/
 
class DecodeStep 
{
protected:
	const DecodeType m_decodeType; /*< either Translate or Generate */
	const Dictionary *m_ptr; /*< pointer to translation/generation table */
	std::vector<bool> m_outputFactorsCovered; /**< mask of the output factors that are covered */

public:
	DecodeStep(DecodeType decodeType, Dictionary *ptr);

	/** returns decoding type, either Translate or Generate */
	DecodeType GetDecodeType() const
	{
		return m_decodeType;
	}

	/** returns phrase table (dictionary) for translation step */
	const PhraseDictionaryBase &GetPhraseDictionary() const
	{
		assert (m_decodeType == Translate);
		return *static_cast<const PhraseDictionaryBase*>(m_ptr);
	}

	/** returns generation table (dictionary) for generation step */
	const GenerationDictionary &GetGenerationDictionary() const
	{
		assert (m_decodeType == Generate);
	  return *static_cast<const GenerationDictionary*>(m_ptr);
	}

	/** returns dictionary in abstract class */
	const Dictionary* GetDictionaryPtr() const {return m_ptr;}
	
};
