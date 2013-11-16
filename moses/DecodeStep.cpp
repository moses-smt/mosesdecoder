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
#include "GenerationDictionary.h"
#include "StaticData.h"
#include "moses/TranslationModel/PhraseDictionary.h"

namespace Moses
{
DecodeStep::DecodeStep(const DecodeFeature *decodeFeature,
                       const DecodeStep* prev,
                       const std::vector<FeatureFunction*> &features)
  : m_decodeFeature(decodeFeature)
{
  FactorMask prevOutputFactors;
  if (prev) prevOutputFactors = prev->m_outputFactors;
  m_outputFactors = prevOutputFactors;
  FactorMask conflictMask = (m_outputFactors & decodeFeature->GetOutputFactorMask());
  m_outputFactors |= decodeFeature->GetOutputFactorMask();
  FactorMask newOutputFactorMask = m_outputFactors ^ prevOutputFactors;  //xor
  m_newOutputFactors.resize(newOutputFactorMask.count());
  m_conflictFactors.resize(conflictMask.count());
  size_t j=0, k=0;
  for (size_t i = 0; i < MAX_NUM_FACTORS; i++) {
    if (newOutputFactorMask[i]) m_newOutputFactors[j++] = i;
    if (conflictMask[i]) m_conflictFactors[k++] = i;
  }
  VERBOSE(2,"DecodeStep():\n\toutputFactors=" << m_outputFactors
          << "\n\tconflictFactors=" << conflictMask
          << "\n\tnewOutputFactors=" << newOutputFactorMask << std::endl);

  // find out which feature function can be applied in this decode step
  for (size_t i = 0; i < features.size(); ++i) {
    FeatureFunction *feature = features[i];
    if (feature->IsUseable(m_outputFactors)) {
      m_featuresToApply.push_back(feature);
    } else {
      m_featuresRemaining.push_back(feature);
    }

  }
}

DecodeStep::~DecodeStep() {}

/** returns phrase feature (dictionary) for translation step */
const PhraseDictionary* DecodeStep::GetPhraseDictionaryFeature() const
{
  return dynamic_cast<const PhraseDictionary*>(m_decodeFeature);
}

/** returns generation feature (dictionary) for generation step */
const GenerationDictionary* DecodeStep::GetGenerationDictionaryFeature() const
{
  return dynamic_cast<const GenerationDictionary*>(m_decodeFeature);
}

void DecodeStep::RemoveFeature(const FeatureFunction *ff)
{
  for (size_t i = 0; i < m_featuresToApply.size(); ++i) {
    if (ff == m_featuresToApply[i]) {
      m_featuresToApply.erase(m_featuresToApply.begin() + i);
      return;
    }
  }
}

}


