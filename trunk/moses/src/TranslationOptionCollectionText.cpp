// $Id$
#include "TranslationOptionCollectionText.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "WordsRange.h"
#include "LMList.h"

using namespace std;

TranslationOptionCollectionText::TranslationOptionCollectionText(Sentence const &inputSentence) 
	: TranslationOptionCollection(inputSentence) {}

void TranslationOptionCollectionText::ProcessInitialTranslation(
															const DecodeStep &decodeStep
															, FactorCollection &factorCollection
															, float weightWordPenalty
															, int dropUnknown
															, size_t verboseLevel
															, PartialTranslOptColl &outputPartialTranslOptColl)
{
	// loop over all substrings of the source sentence, look them up
	// in the phraseDictionary (which is the- possibly filtered-- phrase
	// table loaded on initialization), generate TranslationOption objects
	// for all phrases
	//
	// possible optimization- don't consider phrases longer than the longest
	// phrase in the PhraseDictionary?
	
	PhraseDictionaryBase &phraseDictionary = decodeStep.GetPhraseDictionary();
	for (size_t startPos = 0 ; startPos < m_source.GetSize() ; startPos++)
	{
		if (m_unknownWordPos.GetValue(startPos))
		{ // unknown word but already processed. skip 
			continue;
		}

		// reuse phrase, add next word on
		Phrase sourcePhrase(Input);

		for (size_t endPos = startPos ; endPos < m_source.GetSize() ; endPos++)
		{
			const WordsRange wordsRange(startPos, endPos);

			FactorArray &newWord = sourcePhrase.AddWord();
			Word::Copy(newWord, m_source.GetFactorArray(endPos));

			const TargetPhraseCollection *phraseColl =	phraseDictionary.GetTargetPhraseCollection(sourcePhrase);
			if (phraseColl != NULL)
			{
				if (verboseLevel >= 3) 
				{
					cout << "[" << sourcePhrase << "; " << startPos << "-" << endPos << "]\n";
				}
				
				TargetPhraseCollection::const_iterator iterTargetPhrase;
				for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != phraseColl->end() ; ++iterTargetPhrase)
				{
					const TargetPhrase	&targetPhrase = *iterTargetPhrase;
							
					const WordsRange wordsRange(startPos, endPos);

					TranslationOption transOpt(wordsRange, targetPhrase);
					outputPartialTranslOptColl.push_back ( transOpt );

					if (verboseLevel >= 3) 
					{
						cout << "\t" << targetPhrase << "\n";
					}
				}
				if (verboseLevel >= 3) 
				{ 
					cout << endl; 
				}
			}
			else if (sourcePhrase.GetSize() == 1)
			{
				ProcessUnknownWord(startPos, dropUnknown, factorCollection, weightWordPenalty);
				continue;
			}
		}
	}
}


void TranslationOptionCollectionText::ProcessUnknownWord(size_t sourcePos
																										, int dropUnknown
																										, FactorCollection &factorCollection
																										, float weightWordPenalty)
{
		// unknown word, add to target, and add as poss trans
		//				float	weightWP		= m_staticData.GetWeightWordPenalty();

	const FactorArray &sourceWord = m_source.GetFactorArray(sourcePos);

		size_t isDigit = 0;
		if (dropUnknown)
		{
			const Factor *f = sourceWord[Surface];
			std::string s = f->ToString();
			isDigit = s.find_first_of("0123456789");
			if (isDigit == string::npos) 
				isDigit = 0;
			else 
				isDigit = 1;
			// modify the starting bitmap
		}
		
		TranslationOption *transOpt;
		if (!dropUnknown || isDigit)
		{
			// add to dictionary
			TargetPhrase targetPhraseOrig(Output);
			FactorArray &targetWord = targetPhraseOrig.AddWord();
						
			for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
			{
				FactorType factorType = static_cast<FactorType>(currFactor);
				
				const Factor *sourceFactor = sourceWord[currFactor];
				if (sourceFactor == NULL)
					targetWord[factorType] = factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR);
				else
					targetWord[factorType] = factorCollection.AddFactor(Output, factorType, sourceFactor->GetString());
			}
	
			targetPhraseOrig.SetScore(weightWordPenalty);
			
			pair< set<TargetPhrase>::iterator, bool> inserted = m_unknownTargetPhrase.insert(targetPhraseOrig);
			const TargetPhrase &targetPhrase = *inserted.first;
			transOpt = new TranslationOption(WordsRange(sourcePos, sourcePos), targetPhrase, m_allPhraseDictionary, m_allGenerationDictionary);
		}
		else 
		{ // drop source word. create blank trans opt
			const TargetPhrase targetPhrase(Output);
			transOpt = new TranslationOption(WordsRange(sourcePos, sourcePos), targetPhrase, m_allPhraseDictionary, m_allGenerationDictionary);						
		}

		transOpt->CalcScore(*m_allLM, weightWordPenalty);
		push_back(*transOpt);
		delete transOpt;

		m_unknownWordPos.SetValue(sourcePos, true); 
}
