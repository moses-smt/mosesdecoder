// $Id: DecodeStep.h 149 2007-10-15 21:30:30Z hieu $

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

class PhraseDictionary;
class GenerationDictionary;
class TranslationOption;
class TranslationOptionCollection;
class PartialTranslOptColl;
class InputType;

/** Specification for a decoding step.
 * The factored translation model consists of Translation and Generation
 * steps, which consult a Dictionary of phrase translations or word
 * generations. This class implements the specification for one of these
 * steps, both the DecodeType and a pointer to the Dictionary
 **/
 
class DecodeStep 
{
protected:
	Dictionary *m_ptr; /*< pointer to translation/generation table */

public:
	DecodeStep();
	virtual ~DecodeStep();
	void SetDictionary(Dictionary *ptr);

	//! whether this is Translate or Generate step
	virtual DecodeType GetDecodeType() const = 0;

	virtual void InitializeForInput(InputType const &/*source*/) const;
	virtual void CleanUp() const;

};
