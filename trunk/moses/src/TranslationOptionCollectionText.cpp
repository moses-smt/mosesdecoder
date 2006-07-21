// $Id$
#include "TranslationOptionCollectionText.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "WordsRange.h"

TranslationOptionCollectionText::TranslationOptionCollectionText(Sentence const &inputSentence) 
	: TranslationOptionCollection(inputSentence) {}


int TranslationOptionCollectionText::HandleUnkownWord(PhraseDictionaryBase& phraseDictionary,
																											size_t startPos,
																											FactorCollection &factorCollection,
																											const LMList &allLM,
																											bool dropUnknown,
																											float weightWordPenalty
																											) 
{
	// unknown word, add to target, and add as poss trans
	//				float	weightWP		= m_staticData.GetWeightWordPenalty();
	const FactorTypeSet &targetFactors 		= phraseDictionary.GetFactorsUsed(Output);
	WordsRange wordsRange(startPos,startPos);
	Phrase sourcePhrase(m_source.GetSubString(wordsRange));
	int isDigit = 0;
	if (dropUnknown)
		{
			//const Factor *f = sourcePhrase.GetFactor(0, static_cast<FactorType>(0)); // surface @ 0
			const Factor *f = m_source.GetFactorArray(startPos)[Surface]; 
			isDigit= (f->ToString().find_first_of("0123456789") == std::string::npos);
			// modify the starting bitmap
		}
	if (!dropUnknown || isDigit)
		{
			// add to dictionary
			TargetPhrase targetPhraseOrig(Output, &phraseDictionary);
			FactorArray &targetWord = targetPhraseOrig.AddWord();
		  
			const FactorArray &sourceWord = sourcePhrase.GetFactorArray(0);

			for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
				{
					if (targetFactors.Contains(currFactor))
						{
							FactorType factorType = static_cast<FactorType>(currFactor);
			  
							const Factor *factor = sourceWord[factorType]
								,*unkownfactor;
							switch (factorType)
								{
								case POS:
									unkownfactor = factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR);
									targetWord[factorType] = unkownfactor;
									break;
								default:
									unkownfactor = factorCollection.AddFactor(Output, factorType, factor->GetString());
									targetWord[factorType] = unkownfactor;
									break;
								}
						}
				}
		  
			targetPhraseOrig.SetScore(allLM, weightWordPenalty);
		  
			phraseDictionary.AddEquivPhrase(sourcePhrase, targetPhraseOrig);
			const TargetPhraseCollection *phraseColl = phraseDictionary.GetTargetPhraseCollection(sourcePhrase); //FindEquivPhrase(sourcePhrase);
			assert(phraseColl);
			const TargetPhrase &targetPhrase = *phraseColl->begin();
		  
			TranslationOption transOpt(wordsRange, targetPhrase);
		  
			push_back(transOpt);
		}
	else // drop source word
		{ m_initialCoverage.SetValue(startPos, startPos,1); }
	return 1;
}

