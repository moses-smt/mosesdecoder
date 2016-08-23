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

#ifndef moses_DecodeStep_h
#define moses_DecodeStep_h

#include "TypeDef.h"
#include "FactorTypeSet.h"
#include "Phrase.h"

namespace Moses
{

class DecodeFeature;
class PhraseDictionary;
class GenerationDictionary;
class TranslationOption;
class TranslationOptionCollection;
class PartialTranslOptColl;
class FactorCollection;
class InputType;
class FeatureFunction;
class DecodeGraph;

/** Specification for a decoding step.
 * The factored translation model consists of Translation and Generation
 * steps, which consult a Dictionary of phrase translations or word
 * generations. This class implements the specification for one of these
 * steps, both the DecodeType and a pointer to the Translation or Generation Feature
 **/
class DecodeStep
{
protected:
  FactorMask m_outputFactors; //! mask of what factors exist on the output side after this decode step
  std::vector<FactorType> m_conflictFactors; //! list of the factors that may conflict during this step
  std::vector<FactorType> m_newOutputFactors; //! list of the factors that are new in this step, may be empty
  const DecodeFeature* m_decodeFeature;
  const DecodeGraph *m_container;

  std::vector<FeatureFunction*> m_featuresToApply, m_featuresRemaining;
public:
  DecodeStep(); //! not implemented
  DecodeStep(DecodeFeature *featurePtr,
             const DecodeStep* prevDecodeStep,
             const std::vector<FeatureFunction*> &features);
  virtual ~DecodeStep();

  //! mask of factors that are present after this decode step
  const FactorMask& GetOutputFactorMask() const {
    return m_outputFactors;
  }

  //! returns true if this decode step must match some pre-existing factors
  bool IsFilteringStep() const {
    return !m_conflictFactors.empty();
  }

  //! returns true if this decode step produces one or more new factors
  bool IsFactorProducingStep() const {
    return !m_newOutputFactors.empty();
  }

  const std::vector<FeatureFunction*> &GetFeaturesRemaining() const {
    return m_featuresRemaining;
  }

  /*! returns a list (possibly empty) of the (target side) factors that
   * are produced in this decoding step.  For example, if a previous step
   * generated factor 1, and this step generates 1,2, then only 2 will be
   * in the returned vector. */
  const std::vector<FactorType>& GetNewOutputFactors() const {
    return m_newOutputFactors;
  }

  /*! returns a list (possibly empty) of the (target side) factors that
   * are produced BUT ALREADY EXIST and therefore must be checked for
   * conflict or compatibility */
  const std::vector<FactorType>& GetConflictFactors() const {
    return m_conflictFactors;
  }

  /*! returns phrase table feature for translation step */
  const PhraseDictionary* GetPhraseDictionaryFeature() const;

  /*! returns generation table feature for generation step */
  const GenerationDictionary* GetGenerationDictionaryFeature() const;

  void RemoveFeature(const FeatureFunction *ff);

  void SetContainer(const DecodeGraph *container) {
    m_container = container;
  }
  const DecodeGraph *GetContainer() const {
    return m_container;
  }

};

}
#endif
