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
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "StaticData.h"

size_t DecodeStep::s_id = INITIAL_DECODE_STEP_ID;

DecodeStep::DecodeStep(Dictionary *ptr, const DecodeStep* prev)
:m_ptr(ptr)
,m_id(s_id++)
{
	FactorMask prevCombinedOutputFactors;
	if (prev) prevCombinedOutputFactors = prev->m_combinedOutputFactors;
	m_combinedOutputFactors = prevCombinedOutputFactors;
	FactorMask conflictMask = (m_combinedOutputFactors & ptr->GetOutputFactorMask());
	m_combinedOutputFactors |= ptr->GetOutputFactorMask();
	FactorMask newOutputFactorMask = m_combinedOutputFactors ^ prevCombinedOutputFactors;  //xor
  m_newOutputFactors.resize(newOutputFactorMask.count());
	m_conflictFactors.resize(conflictMask.count());
	size_t j=0, k=0;
  for (size_t i = 0; i < MAX_NUM_FACTORS; i++) {
    if (newOutputFactorMask[i]) m_newOutputFactors[j++] = i;
		if (conflictMask[i]) m_conflictFactors[k++] = i;
	}
  VERBOSE(2,"DecodeStep():\n\toutputFactors=" << m_combinedOutputFactors
	  << "\n\tconflictFactors=" << conflictMask
	  << "\n\tnewOutputFactors=" << newOutputFactorMask << std::endl);
}

DecodeStep::~DecodeStep() {}

/** returns phrase table (dictionary) for translation step */
const PhraseDictionary &DecodeStep::GetPhraseDictionary() const
{
  return *static_cast<const PhraseDictionary*>(m_ptr);
}

/** returns generation table (dictionary) for generation step */
const GenerationDictionary &DecodeStep::GetGenerationDictionary() const
{
  return *static_cast<const GenerationDictionary*>(m_ptr);
}

