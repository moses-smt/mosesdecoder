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

#include "DecodeStepTranslation.h"
#include "PhraseDictionaryMemory.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "PartialTranslOptColl.h"
#include "FactorCollection.h"

DecodeStepTranslation::DecodeStepTranslation(PhraseDictionary* dict, const DecodeStep* prev)
: DecodeStep(dict, prev)
{
}

const PhraseDictionary &DecodeStepTranslation::GetPhraseDictionary() const
{
  return *static_cast<const PhraseDictionary*>(m_ptr);
}

TranslationOption *DecodeStepTranslation::MergeTranslation(const TranslationOption& oldTO, const TargetPhrase &targetPhrase) const
{
  if (IsFilteringStep()) {
    if (!oldTO.IsCompatible(targetPhrase, m_conflictFactors)) return 0;
  }

  TranslationOption *newTransOpt = new TranslationOption(oldTO);
  newTransOpt->MergeNewFeatures(targetPhrase, targetPhrase.GetScoreBreakdown(), m_newOutputFactors);
  return newTransOpt;
}

void DecodeStepTranslation::Process(const InputType &source
															, FactorCollection &factorCollection
															, PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos
															, size_t endPos
															, bool adhereTableLimit) const
{
	const PhraseDictionary &phraseDictionary = GetPhraseDictionary();
	const size_t tableLimit = phraseDictionary.GetTableLimit();

	const WordsRange wordsRange(GetId(), startPos, endPos);
	const TargetPhraseCollection *phraseColl =	phraseDictionary.GetTargetPhraseCollection(source,wordsRange); 

	if (phraseColl != NULL)
	{
		VERBOSE(3,"[" << source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n");
			
		TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
		iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;
		
		for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != iterEnd ; ++iterTargetPhrase)
		{
			const TargetPhrase	&targetPhrase = **iterTargetPhrase;
			outputPartialTranslOptColl.Add ( new TranslationOption(wordsRange, targetPhrase) );
			
			VERBOSE(3,"\t" << targetPhrase << "\n");
		}
		VERBOSE(3,endl);
	}
}

