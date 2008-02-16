// $Id: DecodeStepGeneration.h 149 2007-10-15 21:30:30Z hieu $

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

#include "DecodeStep.h"

class GenerationDictionary;
class Phrase;
class ScoreComponentCollection;

class DecodeStepGeneration : public DecodeStep
{
public:
	DecodeStepGeneration();
	DecodeType GetDecodeType() const
	{
		return Generate;
	}

  /** returns phrase table (dictionary) for translation step */
  const GenerationDictionary &GetGenerationDictionary() const;

	/** mask of factor output by dictionary in this step */
	virtual const FactorMask& GetOutputFactorMask() const
	{
		return m_ptr->GetOutputFactorMask();
	}

  virtual void Process(const TranslationOption &inputPartialTranslOpt
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , bool adhereTableLimit) const;
	bool Load(const std::string &filePath
					, size_t numFeatures
					, const std::vector<FactorType> &input
					, const std::vector<FactorType> &output
					, ScoreIndexManager	&scoreIndexManager);

private:
	/** create new TranslationOption from merging oldTO with mergePhrase
		This function runs IsCompatible() to ensure the two can be merged
	*/
  TranslationOption *MergeGeneration(const TranslationOption& oldTO, Phrase &mergePhrase
                                  , const ScoreComponentCollection& generationScore) const;
};

