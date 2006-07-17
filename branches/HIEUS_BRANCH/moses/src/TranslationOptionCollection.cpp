
#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"

using namespace std;

TranslationOptionCollection::TranslationOptionCollection(const Sentence &inputSentence)
	: m_inputSentence(inputSentence)
	,m_futureScore(inputSentence.GetSize())
	,m_initialCoverage(inputSentence.GetSize())
{
}

/***
 * Add to m_possibleTranslations all possible translations the phrase table gives us for
 * the given phrase
 * 
 * \param phrase The source phrase to translate
 * \param phraseDictionary The phrase table
 * \param lmListInitial A list of language models
 */
void TranslationOptionCollection::CreateTranslationOptions(
															const list < DecodeStep > &decodeStepList
															, const LMList &languageModels
															, const LMList &allLM
															, FactorCollection &factorCollection
															, float weightWordPenalty
															, int dropUnknown
															, size_t verboseLevel)
{
	vector < PartialTranslOptColl > outputPartialTranslOptCollVec( decodeStepList.size() );

	// initial translation step
	list < DecodeStep >::const_iterator iterStep = decodeStepList.begin();
	const DecodeStep &decodeStep = *iterStep;

	ProcessUnknownWords();
	ProcessInitialTranslation(decodeStep, languageModels
														, allLM, factorCollection
														, weightWordPenalty, dropUnknown
														, verboseLevel, outputPartialTranslOptCollVec[0]);

	// do rest of decode steps
	
	int indexStep = 0;
	for (++iterStep ; iterStep != decodeStepList.end() ; ++iterStep) 
	{
		const DecodeStep &decodeStep = *iterStep;
		PartialTranslOptColl &inputPartialTranslOptColl		= outputPartialTranslOptCollVec[indexStep]
												,&outputPartialTranslOptColl	= outputPartialTranslOptCollVec[indexStep + 1];

		// is it translation or generation
		switch (decodeStep.GetDecodeType()) 
		{
		case Translate:
			{
				// go thru each intermediate trans opt just created
				PartialTranslOptColl::const_iterator iterPartialTranslOpt;
				for (iterPartialTranslOpt = inputPartialTranslOptColl.begin() ; iterPartialTranslOpt != inputPartialTranslOptColl.end() ; ++iterPartialTranslOpt)
				{
					const PartialTranslOpt &inputPartialTranslOpt = *iterPartialTranslOpt;
					ProcessTranslation(inputPartialTranslOpt, decodeStep, outputPartialTranslOptColl);
				}
				break;
			}
		case Generate:
			{
				// go thru each hypothesis just created
				PartialTranslOptColl::const_iterator iterPartialTranslOpt;
				for (iterPartialTranslOpt = inputPartialTranslOptColl.begin() ; iterPartialTranslOpt != inputPartialTranslOptColl.end() ; ++iterPartialTranslOpt)
				{
					const PartialTranslOpt &inputPartialTranslOpt = *iterPartialTranslOpt;
					ProcessGeneration(inputPartialTranslOpt, decodeStep, outputPartialTranslOptColl);
				}
				break;
			}
		}

		indexStep++;
	} // for (++iterStep 

}

void TranslationOptionCollection::ProcessUnknownWords()
{
}

void TranslationOptionCollection::ProcessInitialTranslation(
															const DecodeStep &decodeStep
															, const LMList &languageModels
															, const LMList &allLM
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
	
	PhraseDictionary &phraseDictionary = decodeStep.GetPhraseDictionary();
	for (size_t startPos = 0 ; startPos < m_inputSentence.GetSize() ; startPos++)
	{
		// reuse phrase, add next word on
		Phrase sourcePhrase( m_inputSentence.GetDirection());

		for (size_t endPos = startPos ; endPos < m_inputSentence.GetSize() ; endPos++)
		{
			const WordsRange wordsRange(startPos, endPos);

			FactorArray &newWord = sourcePhrase.AddWord();
			Word::Copy(newWord, m_inputSentence.GetFactorArray(endPos));

			const TargetPhraseCollection *phraseColl =	phraseDictionary.FindEquivPhrase(sourcePhrase);
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

					outputPartialTranslOptColl.push_back ( PartialTranslOpt(wordsRange, targetPhrase) );

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
				// unknown word, add to target, and add as poss trans
				//				float	weightWP		= m_staticData.GetWeightWordPenalty();
				const FactorTypeSet &targetFactors 		= phraseDictionary.GetFactorsUsed(Output);
				size_t isDigit = 0;
				if (dropUnknown)
				{
					const Factor *f = sourcePhrase.GetFactor(0, static_cast<FactorType>(0)); // surface @ 0
					std::string s = f->ToString();
					isDigit = s.find_first_of("0123456789");
					if (isDigit == string::npos) 
						isDigit = 0;
					else 
						isDigit = 1;
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
					const TargetPhraseCollection *phraseColl = phraseDictionary.FindEquivPhrase(sourcePhrase);
					const TargetPhrase &targetPhrase = *phraseColl->begin();
					
					outputPartialTranslOptColl.push_back( PartialTranslOpt(wordsRange, targetPhrase) );
				}
				else // drop source word
				{ 
					m_initialCoverage.SetValue(startPos, startPos,1); 
				}
			}
		}
	}

	// create future score matrix
	// for each span in the source phrase (denoted by start and end)
	for(size_t startPos = 0; startPos < m_inputSentence.GetSize() ; startPos++) 
	{
		for(size_t endPos = startPos; endPos < m_inputSentence.GetSize() ; endPos++) 
		{
			size_t length = endPos - startPos + 1;
			vector< float > score(length + 1);
			score[0] = 0;
			for(size_t currLength = 1 ; currLength <= length ; currLength++) 
			// initalize their future cost to -infinity
			{
				score[currLength] = - numeric_limits<float>::infinity();
			}

			for(size_t currLength = 0 ; currLength < length ; currLength++) 
			{
				// iterate over possible translations of this source subphrase and
				// keep track of the highest cost option
				TranslationOptionCollection::const_iterator iterTransOpt;
				for(iterTransOpt = begin() ; iterTransOpt != end() ; ++iterTransOpt)
				{
					const TranslationOption &transOpt = *iterTransOpt;
					size_t index = currLength + transOpt.GetSize();

					if (transOpt.GetStartPos() == currLength + startPos 
							&& transOpt.GetEndPos() <= endPos 
							&& transOpt.GetFutureScore() + score[currLength] > score[index]) 
					{
						score[index] = transOpt.GetFutureScore() + score[currLength];
					}
				}
			}
			// record the highest cost option in the future cost table.
			m_futureScore.SetScore(startPos, endPos, score[length]);

			//print information about future cost table when verbose option is set

			if(verboseLevel > 0) 
			{		
				cout<<"future cost from "<<startPos<<" to "<<endPos<<" is "<<score[length]<<endl;
			}
		}
	}
}

void TranslationOptionCollection::ProcessTranslation(
								const PartialTranslOpt &inputPartialTranslOpt
								, const DecodeStep		 &decodeStep
								, PartialTranslOptColl &outputPartialTranslOptColl)
{
	const TargetPhrase &partialPhrase					= inputPartialTranslOpt.GetTargetPhrase();
	const WordsRange &sourceWordsRange				= inputPartialTranslOpt.GetSourceWordsRange();
	const Phrase sourcePhrase 								= m_inputSentence.GetSubString(sourceWordsRange);
	const PhraseDictionary &phraseDictionary	= decodeStep.GetPhraseDictionary();

	const TargetPhraseCollection *phraseColl	=	phraseDictionary.FindEquivPhrase(sourcePhrase);

	if (phraseColl != NULL)
	{
		TargetPhraseCollection::const_iterator iterTargetPhrase;

		for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != phraseColl->end(); ++iterTargetPhrase)
		{
			const TargetPhrase& targetPhrase	= *iterTargetPhrase;
	
			TargetPhrase *newTargetPhrase = partialPhrase.MergeNext(targetPhrase);
			
			if (newTargetPhrase != NULL)
			{
				outputPartialTranslOptColl.Add( PartialTranslOpt(sourceWordsRange, *newTargetPhrase) );
				delete newTargetPhrase;
			}
		}
	}
	else if (sourceWordsRange.GetWordsCount() == 1)
	{ // another unknown handler here
		// ??? unknown word handler must check for unknown factor across all factor types for this to be unecessary
	}
}

// helpers
typedef pair<Word, float> WordPair;
typedef list< WordPair > WordList;	
	// 1st = word 
	// 2nd = score
typedef list< WordPair >::const_iterator WordListIterator;

inline void IncrementIterators(vector< WordListIterator > &wordListIterVector
												, const vector< WordList > &wordListVector)
{
	for (size_t currPos = 0 ; currPos < wordListVector.size() ; currPos++)
	{
		WordListIterator &iter = wordListIterVector[currPos];
		iter++;
		if (iter != wordListVector[currPos].end())
		{ // eg. 4 -> 5
			return;
		}
		else
		{ //  eg 9 -> 10
			iter = wordListVector[currPos].begin();
		}
	}
}

void TranslationOptionCollection::ProcessGeneration(			
														const PartialTranslOpt &inputPartialTranslOpt
														, const DecodeStep &decodeStep
														, PartialTranslOptColl &outputPartialTranslOptColl)
{
	const GenerationDictionary &generationDictionary	= decodeStep.GetGenerationDictionary();
	const WordsRange &sourceWordsRange								= inputPartialTranslOpt.GetSourceWordsRange();
	const float weight																= generationDictionary.GetWeight();

	const TargetPhrase &targetPhrase = inputPartialTranslOpt.GetTargetPhrase();
	size_t targetLength	= targetPhrase.GetSize();

	// generation list for each word in hypothesis
	vector< WordList > wordListVector(targetLength);

	// create generation list
	int wordListVectorPos = 0;
	for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
	{
		WordList &wordList = wordListVector[wordListVectorPos];
		const FactorArray &factorArray = targetPhrase.GetFactorArray(currPos);

		const OutputWordCollection *wordColl = generationDictionary.FindWord(factorArray);

		if (wordColl == NULL)
		{	// word not found in generation dictionary
			// go no further
			return;
		}

		OutputWordCollection::const_iterator iterWordColl;
		for (iterWordColl = wordColl->begin() ; iterWordColl != wordColl->end(); ++iterWordColl)
		{
			const Word &outputWord = (*iterWordColl).first;
			float score = (*iterWordColl).second;
			wordList.push_back(WordPair(outputWord, score));
		}
		
		wordListVectorPos++;
	}

	// use generation list (wordList)
	// set up iterators
	size_t numIteration = 1;
	vector< WordListIterator >	wordListIterVector(targetLength);
	vector< const Word* >				mergeWords(targetLength);
	for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
	{
		wordListIterVector[currPos] = wordListVector[currPos].begin();
		numIteration *= wordListVector[currPos].size();
	}

	// go thru each possible factor for each word & create hypothesis
	for (size_t currIter = 0 ; currIter < numIteration ; currIter++)
	{
		float generationScore = 0; // total score for this string of words

		// create vector of words with new factors for last phrase
		for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
		{
			const WordPair &wordPair = *wordListIterVector[currPos];
			mergeWords[currPos] = &(wordPair.first);
			generationScore += wordPair.second;
		}

		// merge with existing phrase
		TargetPhrase newTargetPhrase(targetPhrase);
		newTargetPhrase.MergeFactors(mergeWords, generationDictionary, generationScore, weight);
		outputPartialTranslOptColl.Add(PartialTranslOpt(sourceWordsRange, newTargetPhrase));

		// increment iterators
		IncrementIterators(wordListIterVector, wordListVector);
	}
}

