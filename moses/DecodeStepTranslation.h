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

#ifndef moses_DecodeStepTranslation_h
#define moses_DecodeStepTranslation_h

#include "DecodeStep.h"
#include "moses/TranslationModel/PhraseDictionary.h"

namespace Moses
{

class PhraseDictionary;
class TargetPhrase;

//! subclass of DecodeStep for translation step
class DecodeStepTranslation : public DecodeStep
{
public:
  DecodeStepTranslation(); //! not implemented
  DecodeStepTranslation(const PhraseDictionary* phraseFeature,
                        const DecodeStep* prev,
                        const std::vector<FeatureFunction*> &features);


  virtual void Process(const TranslationOption &inputPartialTranslOpt
                       , const DecodeStep &decodeStep
                       , PartialTranslOptColl &outputPartialTranslOptColl
                       , TranslationOptionCollection *toc
                       , bool adhereTableLimit
                       , const Phrase &src) const;
  virtual void Process(const TranslationOption &inputPartialTranslOpt
                       , const DecodeStep &decodeStep
                       , PartialTranslOptColl &outputPartialTranslOptColl
                       , TranslationOptionCollection *toc
                       , bool adhereTableLimit
                       , const Phrase &src
                       , const TargetPhraseCollection *phraseColl) const;


  /*! initialize list of partial translation options by applying the first translation step
  * Ideally, this function should be in DecodeStepTranslation class
  */
  void ProcessInitialTranslationLegacy(const InputType &source
                                 , PartialTranslOptColl &outputPartialTranslOptColl
                                 , size_t startPos, size_t endPos, bool adhereTableLimit) const;

  void ProcessInitialTranslation(const InputType &source
                                 , PartialTranslOptColl &outputPartialTranslOptColl
                                 , size_t startPos, size_t endPos, bool adhereTableLimit
                                 , const TargetPhraseCollection *phraseColl) const;

private:

};


}
#endif
