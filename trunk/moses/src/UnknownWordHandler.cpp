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

#include "StaticData.h"
#include "TranslationOption.h"
#include "UnknownWordHandler.h"

/***
 * default implementation: assume the word/phrase is a proper noun and set it as its own translation
 */
boost::shared_ptr<std::list<TranslationOption> > UnknownWordHandler::GetPossibleTranslations(
	const WordsRange& sourceWordsRange, const Phrase& sourcePhrase, StaticData& staticData, PhraseDictionary& phraseDictionary) const
{
	TargetPhrase targetPhrase(Output, &phraseDictionary);
	FactorArray &targetWord = targetPhrase.AddWord();
	const FactorArray &sourceWord = sourcePhrase.GetFactorArray(0);
	
	//start processing source phrase: here, just copy factors to target
	const FactorTypeSet &targetFactors = phraseDictionary.GetFactorsUsed(Output);
	for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
		if (targetFactors.Contains(currFactor))
		{
			FactorType factorType = static_cast<FactorType>(currFactor);

			const Factor *factor = sourceWord[factorType], *unknownFactor;
			switch (factorType)
			{
			case POS:
				unknownFactor = staticData.GetFactorCollection().AddFactor(Output, factorType, UNKNOWN_FACTOR);
				targetWord[factorType] = unknownFactor;
				break;
			default:
				unknownFactor = staticData.GetFactorCollection().AddFactor(Output, factorType, factor->GetString());
				targetWord[factorType] = unknownFactor;
				break;
			}
		}
	}
	LMList languageModels = staticData.GetAllLM();
	targetPhrase.SetScore(languageModels, staticData.GetWeightWordPenalty());
	
	/*
	 * add possible translations to the phrase table
	 * (so that if we hit this source phrase again, we won't reprocess it because it won't still be unknown)
	 */
	phraseDictionary.AddEquivPhrase(sourcePhrase, targetPhrase);
	
	//turn phrase-table entries into TranslationOption objects
	const TargetPhraseCollection *phraseColl = phraseDictionary.GetTargetPhraseCollection(sourcePhrase);
	boost::shared_ptr<std::list<TranslationOption> > transOpts(new std::list<TranslationOption>);
	for(TargetPhraseCollection::const_iterator i = phraseColl->begin(); i != phraseColl->end(); i++)
		transOpts->push_back(TranslationOption(sourceWordsRange, *i));
	return transOpts;
}
