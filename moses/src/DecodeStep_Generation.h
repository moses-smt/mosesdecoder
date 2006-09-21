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

#ifndef _Generation_DECODE_STEP_H_
#define _Generation_DECODE_STEP_H_

#include "DecodeStep.h"
#include "Word.h"
#include "ScoreComponentCollection.h"

class GenerationDictionary;
class Phrase;
class WordsRange;
class ScoreComponentCollection2;

class GenerationDecodeStep : public DecodeStep
{
public:
	GenerationDecodeStep(GenerationDictionary* dict, const DecodeStep* prev);

	const int GetType() const { return 1; };

  /** returns phrase table (dictionary) for translation step */
  const GenerationDictionary &GetGenerationDictionary() const;

	int GenerateOptions(std::vector<WordList>& wordListVector, const Phrase& targetPhrase);
  virtual void Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , FactorCollection &factorCollection
                              , TranslationOptionCollection *toc) const;

private:
  TranslationOption *MergeGeneration(const TranslationOption& oldTO, Phrase &mergePhrase
                                  , const ScoreComponentCollection2& generationScore) const;

};

#endif
