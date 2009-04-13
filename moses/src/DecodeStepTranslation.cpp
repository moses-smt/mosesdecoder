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
#include "IntraPhraseManager.h"

size_t DecodeStepTranslation::s_id = 0;

DecodeStepTranslation::DecodeStepTranslation(PhraseDictionary* dict, const DecodeStep* prev)
: DecodeStep(dict, prev)
, m_id(s_id++)
{
}

const PhraseDictionary &DecodeStepTranslation::GetPhraseDictionary() const
{
  return *static_cast<const PhraseDictionary*>(m_ptr);
}

TranslationOption *DecodeStepTranslation::MergeTranslation(const TranslationOption& oldTO, const TargetPhrase &targetPhrase) const
{
  if (!oldTO.GetTargetPhrase().IsCompatible(targetPhrase, m_conflictFactors)) 
		return NULL;

  TranslationOption *newTransOpt = new TranslationOption(oldTO);
  newTransOpt->MergeTargetPhrase(targetPhrase
																, targetPhrase.GetScoreBreakdown()
																, m_newOutputFactors
																, GetId());
  return newTransOpt;
}

void DecodeStepTranslation::Process(const TranslationOption &inputPartialTranslOpt
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , TranslationOptionCollection *toc
                              , bool adhereTableLimit) const
{
	assert(false);
}

void DecodeStepTranslation::Process(const WordsRange &sourceRange
															, const std::vector<TranslationOption*> &inputPartialTranslOptList
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , TranslationOptionCollection *toc
                              , bool adhereTableLimit) const
{
	if (inputPartialTranslOptList.size() == 0)
		return;

	/* TODO - put back in
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
  { // word deletion
    outputPartialTranslOptColl.Add(new TranslationOption(inputPartialTranslOpt));
    return;
  }
	*/

	IntraPhraseManager intraPhraseManager(inputPartialTranslOptList
																			, sourceRange
																			, toc->GetSource()
																			, GetPhraseDictionary());
	const IntraPhraseHypothesisStack &phraseColl = intraPhraseManager.GetTargetPhraseCollection();
	
	IntraPhraseHypothesisStack::const_iterator iter;
	for (iter = phraseColl.begin() ; iter != phraseColl.end() ; ++iter)
	{
		const IntraPhraseTargetPhrase &targetPhrase = **iter;
		const std::vector<TranslationOption*> &transOptList = targetPhrase.GetTranslationOptionList();

		std::vector<TranslationOption*>::const_iterator iterTransOptList;
		for (iterTransOptList = transOptList.begin() ; iterTransOptList != transOptList.end() ; ++iterTransOptList)
		{	
			const TranslationOption &transOpt = **iterTransOptList;
			
			TranslationOption *newTransOpt = MergeTranslation(transOpt, targetPhrase);
			assert(newTransOpt != NULL);
			outputPartialTranslOptColl.Add(newTransOpt);
		}
	}
}

void DecodeStepTranslation::ProcessInitialTranslation(
															const InputType &source
															,PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos, size_t endPos, bool adhereTableLimit
															, const TargetPhraseCollection *mustKeepPhrases
															, const DecodeGraph &decodeGraph) const
{
	const PhraseDictionary &phraseDictionary = GetPhraseDictionary();
	const size_t tableLimit = phraseDictionary.GetTableLimit();

	const WordsRange wordsRange(startPos, endPos);
	const TargetPhraseCollection *phraseColl =	phraseDictionary.GetTargetPhraseCollection(source,wordsRange); 

	if (phraseColl != NULL)
	{
		VERBOSE(3,"[" << source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n");
			
		TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
		iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;
		
		for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != iterEnd ; ++iterTargetPhrase)
		{
			const TargetPhrase	&targetPhrase = **iterTargetPhrase;
			outputPartialTranslOptColl.AddNoPrune ( new TranslationOption(wordsRange, targetPhrase, source, 0, decodeGraph) );
			
			VERBOSE(3,"\t" << targetPhrase << "\n");
		}
		VERBOSE(3,endl);
	}
}


