#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "Input.h"

using namespace std;

TranslationOptionCollection::TranslationOptionCollection(InputType const& src)
	: m_source(src)
	,m_futureScore(src.GetSize())
	,m_unknownWordPos(src.GetSize())
{
}

TranslationOptionCollection::~TranslationOptionCollection()
{
}

void TranslationOptionCollection::CalcFutureScore(size_t verboseLevel)
{
	// create future score matrix
	// for each span in the source phrase (denoted by start and end)
	for(size_t startPos = 0; startPos < m_source.GetSize() ; startPos++) 
		{
			for(size_t endPos = startPos; endPos < m_source.GetSize() ; endPos++) 
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
																										const TranslationOption &inputPartialTranslOpt
																										, const DecodeStep &decodeStep
																										, PartialTranslOptColl &outputPartialTranslOptColl
																										, int dropUnknown
																										, FactorCollection &factorCollection
																										, float weightWordPenalty)
{
	//TRACE_ERR(inputPartialTranslOpt << endl);
	if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
		{ // word deletion
		
			TranslationOption newTransOpt(inputPartialTranslOpt);
#ifdef N_BEST
			const GenerationDictionary &dictionary	= decodeStep.GetGenerationDictionary();
			newTransOpt.AddGenScoreComponent(dictionary, 0.0f);
#endif
			outputPartialTranslOptColl.Add(newTransOpt);
		
			return;
		}
	
	// normal generation step
	const GenerationDictionary &generationDictionary	= decodeStep.GetGenerationDictionary();
	const WordsRange &sourceWordsRange								= inputPartialTranslOpt.GetSourceWordsRange();
	const float weight																= generationDictionary.GetWeight();

	const Phrase &targetPhrase	= inputPartialTranslOpt.GetTargetPhrase();
	size_t targetLength					= targetPhrase.GetSize();

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
					ProcessUnknownWord(sourceWordsRange.GetStartPos(), dropUnknown, factorCollection, weightWordPenalty);
					return;
				}
			else
				{
					OutputWordCollection::const_iterator iterWordColl;
					for (iterWordColl = wordColl->begin() ; iterWordColl != wordColl->end(); ++iterWordColl)
						{
							const Word &outputWord = (*iterWordColl).first;
							float score = (*iterWordColl).second;
							wordList.push_back(WordPair(outputWord, score));
						}
		
					wordListVectorPos++;
				}
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

			// merge with existing trans opt
			Phrase genPhrase(Output, mergeWords);
			TranslationOption *newTransOpt = inputPartialTranslOpt.MergeGeneration(genPhrase, &generationDictionary, generationScore, weight);
			if (newTransOpt != NULL)
				{
					outputPartialTranslOptColl.Add( *newTransOpt );
					delete newTransOpt;
				}

			// increment iterators
			IncrementIterators(wordListIterVector, wordListVector);
		}
}


void TranslationOptionCollection::ProcessTranslation(
																										 const TranslationOption &inputPartialTranslOpt
																										 , const DecodeStep		 &decodeStep
																										 , PartialTranslOptColl &outputPartialTranslOptColl
																										 , int dropUnknown
																										 , FactorCollection &factorCollection
																										 , float weightWordPenalty)
{
	//TRACE_ERR(inputPartialTranslOpt << endl);
	if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
		{ // word deletion
		
			TranslationOption newTransOpt(inputPartialTranslOpt);
#ifdef N_BEST
			const PhraseDictionaryBase &phraseDictionary	= decodeStep.GetPhraseDictionary();
			ScoreComponent transComp(&phraseDictionary);
			transComp.Reset();
			newTransOpt.AddTransScoreComponent(transComp);
#endif
			outputPartialTranslOptColl.Add(newTransOpt);
		
			return;
		}
	
	// normal trans step
	const WordsRange &sourceWordsRange				= inputPartialTranslOpt.GetSourceWordsRange();
	const PhraseDictionaryBase &phraseDictionary	= decodeStep.GetPhraseDictionary();
	const TargetPhraseCollection *phraseColl	=	phraseDictionary.GetTargetPhraseCollection(m_source,sourceWordsRange); 
	
	if (phraseColl != NULL)
		{
			TargetPhraseCollection::const_iterator iterTargetPhrase;

			for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != phraseColl->end(); ++iterTargetPhrase)
				{
					const TargetPhrase& targetPhrase	= *iterTargetPhrase;
			
					TranslationOption *newTransOpt = inputPartialTranslOpt.MergeTranslation(targetPhrase);
					if (newTransOpt != NULL)
						{
							outputPartialTranslOptColl.Add( *newTransOpt );
							delete newTransOpt;
						}
				}
		}
	else if (sourceWordsRange.GetWordsCount() == 1)
		{ // unknown handler
			ProcessUnknownWord(sourceWordsRange.GetStartPos(), dropUnknown, factorCollection, weightWordPenalty);
		}
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
																													 , const LMList &allLM
																													 , FactorCollection &factorCollection
																													 , float weightWordPenalty
																													 , bool dropUnknown
																													 , size_t verboseLevel)
{
	m_allLM = &allLM;
	vector < PartialTranslOptColl > outputPartialTranslOptCollVec( decodeStepList.size() );

	// fill list of dictionaries for unknown word handling
	{
		list<DecodeStep>::const_iterator iter;
		for (iter = decodeStepList.begin() ; iter != decodeStepList.end() ; ++iter)
			{
				switch (iter->GetDecodeType())
					{
					case Translate:
						m_allPhraseDictionary.push_back(&(iter->GetPhraseDictionary()));
						break;
					case Generate:
						m_allGenerationDictionary.push_back(&(iter->GetGenerationDictionary()));
						break;
					}
			}
	}

	// initial translation step
	list < DecodeStep >::const_iterator iterStep = decodeStepList.begin();
	const DecodeStep &decodeStep = *iterStep;

	ProcessInitialTranslation(decodeStep, factorCollection
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
								const TranslationOption &inputPartialTranslOpt = *iterPartialTranslOpt;
								ProcessTranslation(inputPartialTranslOpt
																	 , decodeStep
																	 , outputPartialTranslOptColl
																	 , dropUnknown
																	 , factorCollection
																	 , weightWordPenalty);
							}
						break;
					}
				case Generate:
					{
						// go thru each hypothesis just created
						PartialTranslOptColl::const_iterator iterPartialTranslOpt;
						for (iterPartialTranslOpt = inputPartialTranslOptColl.begin() ; iterPartialTranslOpt != inputPartialTranslOptColl.end() ; ++iterPartialTranslOpt)
							{
								const TranslationOption &inputPartialTranslOpt = *iterPartialTranslOpt;
								ProcessGeneration(inputPartialTranslOpt
																	, decodeStep
																	, outputPartialTranslOptColl
																	, dropUnknown
																	, factorCollection
																	, weightWordPenalty);
							}
						break;
					}
				}
			indexStep++;
		} // for (++iterStep 

	// add to real trans opt list
	PartialTranslOptColl &lastPartialTranslOptColl	= outputPartialTranslOptCollVec[decodeStepList.size() - 1];
	iterator iterColl;
	for (iterColl = lastPartialTranslOptColl.begin() ; iterColl != lastPartialTranslOptColl.end() ; iterColl++)
		{
			TranslationOption &transOpt = *iterColl;
			transOpt.CalcScore(allLM, weightWordPenalty);
			Add(transOpt);
		}

	// future score
	CalcFutureScore(verboseLevel);
}
