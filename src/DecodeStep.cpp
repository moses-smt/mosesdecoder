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

#include "DecodeStep.h"
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"

DecodeStep::DecodeStep(DecodeType decodeType, Dictionary *ptr, const DecodeStep* prev)
:m_decodeType(decodeType)
,m_ptr(ptr)
{
	if (prev) m_outputFactors = prev->m_outputFactors;
	m_conflictMask = (m_outputFactors & ptr->GetOutputFactorMask());
	m_outputFactors |= ptr->GetOutputFactorMask();
}

/** returns phrase table (dictionary) for translation step */
const PhraseDictionaryBase &DecodeStep::GetPhraseDictionary() const
{
  assert (m_decodeType == Translate);
  return *static_cast<const PhraseDictionaryBase*>(m_ptr);
}

/** returns generation table (dictionary) for generation step */
const GenerationDictionary &DecodeStep::GetGenerationDictionary() const
{
  assert (m_decodeType == Generate);
  return *static_cast<const GenerationDictionary*>(m_ptr);
}

