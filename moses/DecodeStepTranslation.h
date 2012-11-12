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
#include "PhraseDictionary.h"

namespace Moses
{

class PhraseDictionaryFeature;
class TargetPhrase;

//! subclass of DecodeStep for translation step
class DecodeStepTranslation : public DecodeStep
{
public:
  DecodeStepTranslation(); //! not implemented
  DecodeStepTranslation(const PhraseDictionaryFeature* phraseFeature, const DecodeStep* prev);


  virtual void Process(const TranslationSystem* system
                       , const TranslationOption &inputPartialTranslOpt
                       , const DecodeStep &decodeStep
                       , PartialTranslOptColl &outputPartialTranslOptColl
                       , TranslationOptionCollection *toc
                       , bool adhereTableLimit) const;


  /*! initialize list of partial translation options by applying the first translation step
  * Ideally, this function should be in DecodeStepTranslation class
  */
  void ProcessInitialTranslation(const TranslationSystem* system
                                 , const InputType &source
                                 , PartialTranslOptColl &outputPartialTranslOptColl
                                 , size_t startPos, size_t endPos, bool adhereTableLimit) const;

private:
  /*! create new TranslationOption from merging oldTO with mergePhrase
  	This function runs IsCompatible() to ensure the two can be merged
  */
  TranslationOption *MergeTranslation(const TranslationOption& oldTO, const TargetPhrase &targetPhrase) const;
};


}
#endif
