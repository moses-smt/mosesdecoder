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
#include "Dictionary.h"

class PhraseDictionaryBase;
class GenerationDictionary;

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
	FactorMask m_outputFactors; /** mask of what factors exist on the output side after this decode step */
	FactorMask m_conflictMask; /** mask of what factors might conflict during this step */

public:
	DecodeStep(DecodeType decodeType, Dictionary *ptr, const DecodeStep* prevDecodeStep);

	/** returns decoding type, either Translate or Generate */
	DecodeType GetDecodeType() const
	{
		return m_decodeType;
	}

	/** mask of factors that may conflict during this decode step */
	const FactorMask& GetConflictFactorMask() const
	{
		return m_conflictMask;
	}

	/** mask of factors that are present after this decode step */
	const FactorMask& GetOutputFactorMask() const
	{
		return m_outputFactors;
	}

	/** returns phrase table (dictionary) for translation step */
	const PhraseDictionaryBase &GetPhraseDictionary() const;

	/** returns generation table (dictionary) for generation step */
	const GenerationDictionary &GetGenerationDictionary() const;

	/** returns dictionary in abstract class */
	const Dictionary* GetDictionaryPtr() const {return m_ptr;}
	
};
