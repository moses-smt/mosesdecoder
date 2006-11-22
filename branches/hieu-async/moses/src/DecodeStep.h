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

class PhraseDictionary;
class GenerationDictionary;
class TranslationOption;
class TranslationOptionCollection;
class PartialTranslOptColl;
class FactorCollection;
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
	const Dictionary *m_ptr; /*< pointer to translation/generation table */
	FactorMask m_outputFactors; /** mask of what factors exist on the output side after this decode step */
	std::vector<FactorType> m_conflictFactors; /** list of the factors that may conflict during this step*/
	std::vector<FactorType> m_newOutputFactors; /** list of the factors that are new in this step, may be empty*/

public:
	DecodeStep(Dictionary *ptr, const DecodeStep* prevDecodeStep);
	virtual ~DecodeStep();

	/** mask of factors that are present after this decode step */
	const FactorMask& GetOutputFactorMask() const
	{
		return m_outputFactors;
	}

	//! returns true if this decode step must match some pre-existing factors
	bool IsFilteringStep() const
	{
		return !m_conflictFactors.empty();
	}

	//! returns true if this decode step produces one or more new factors
	bool IsFactorProducingStep() const
	{
		return !m_newOutputFactors.empty();
	}

	/** returns a list (possibly empty) of the (target side) factors that
	 * are produced in this decoding step.  For example, if a previous step
	 * generated factor 1, and this step generates 1,2, then only 2 will be
	 * in the returned vector. */
	const std::vector<FactorType>& GetNewOutputFactors() const
	{
		return m_newOutputFactors;
	}

	/** returns a list (possibly empty) of the (target side) factors that
	 * are produced BUT ALREADY EXIST and therefore must be checked for
	 * conflict or compatibility */
	const std::vector<FactorType>& GetConflictFactors() const
	{
		return m_conflictFactors;
	}

	/** returns phrase table (dictionary) for translation step */
	const PhraseDictionary &GetPhraseDictionary() const;

	/** returns generation table (dictionary) for generation step */
	const GenerationDictionary &GetGenerationDictionary() const;

	/** returns dictionary in abstract class */
	const Dictionary* GetDictionaryPtr() const {return m_ptr;}

	/** Given an input TranslationOption, extend it in some way (put results in outputPartialTranslOptColl) */
  virtual void Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , FactorCollection &factorCollection
															, TranslationOptionCollection *toc
															, bool adhereTableLimit) const = 0;

};
