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
#include "TargetPhraseMatrix.h"

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
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
  { // word deletion
    outputPartialTranslOptColl.Add(new TranslationOption(inputPartialTranslOpt));
    return;
  }

	// create	trans option
	ConcatenatedPhraseColl::const_iterator iterConcatePhrase;
	for (iterConcatePhrase = m_concatenatedPhraseColl->begin(); iterConcatePhrase != m_concatenatedPhraseColl->end(); ++iterConcatePhrase)	
  {
		const ConcatenatedPhrase &concatePhrase = *iterConcatePhrase;
    
    if (concatePhrase.GetPhraseSize() == inputPartialTranslOpt.GetTargetPhrase().GetSize())
    { // same size
      // create all possible perm of this concate phrase
      const TargetPhraseCollection &targetPhraseColl
                = concatePhrase.CreateTargetPhrases();

      TargetPhraseCollection::const_iterator iterTargetPhrase;
      for (iterTargetPhrase = targetPhraseColl.begin(); iterTargetPhrase != targetPhraseColl.end(); ++iterTargetPhrase)
      {
        const TargetPhrase &targetPhrase = **iterTargetPhrase;
        
        TranslationOption *newTransOpt = MergeTranslation(inputPartialTranslOpt, targetPhrase);
        if (newTransOpt != NULL)
		    {			    	
			    outputPartialTranslOptColl.Add(newTransOpt);
		    }
      }
    }
	}
}

void DecodeStepTranslation::CreateTargetPhrases(
																ConcatenatedPhraseColl &concatenatedPhraseColl
															, const WordsRange &sourceWordsRange
                              , TranslationOptionCollection *toc
                              , bool adhereTableLimit) const
{
  const PhraseDictionary &phraseDictionary  = GetPhraseDictionary();
	const size_t tableLimit = phraseDictionary.GetTableLimit();
	const size_t sourceSize = sourceWordsRange.GetNumWordsCovered();

	TargetPhraseMatrix targetPhraseMatrix(sourceSize);
	
	size_t sourceStartPos = sourceWordsRange.GetStartPos();

	// grab all the sub range trans
	for (size_t startPos = sourceStartPos; startPos <= sourceWordsRange.GetEndPos(); ++startPos)
	{
		for (size_t endPos = startPos; endPos <= sourceWordsRange.GetEndPos(); ++endPos)
		{
			WordsRange subRange(startPos, endPos);
		  const TargetPhraseCollection *phraseColl= phraseDictionary.GetTargetPhraseCollection(toc->GetSource(),subRange);
			targetPhraseMatrix.Add(startPos - sourceStartPos, endPos - sourceStartPos, phraseColl);
		}
	}

	// concatenate target phrase together
	targetPhraseMatrix.CreateConcatenatedPhraseList(concatenatedPhraseColl, tableLimit, adhereTableLimit);
}

void DecodeStepTranslation::ProcessInitialTranslation(
															const InputType &source
															,PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos, size_t endPos, bool adhereTableLimit) const
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
			outputPartialTranslOptColl.AddNoPrune ( new TranslationOption(wordsRange, targetPhrase, source, 0) );
			
			VERBOSE(3,"\t" << targetPhrase << "\n");
		}
		VERBOSE(3,endl);
	}
}


