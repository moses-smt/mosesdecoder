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
	// create future score matrix in a dynamic programming fashion


    // setup the matrix (ignore lower triangle, set upper triangle to -inf
    size_t size = m_source.GetSize(); // the width of the matrix

    // counting options per span, for statistics
    bool printCounts = (verboseLevel > 0);
    int *counts;
    if (printCounts == true)
      counts = (int*) malloc(sizeof(int) * size * size);

    for(size_t row=0; row<size; row++) {
      for(size_t col=row; col<size; col++) {
        m_futureScore.SetScore(row, col, -numeric_limits<float>::infinity());
        if (printCounts == true) counts[row*size+col] = 0;
      }
    }

    // walk all the translation options and record the cheapest option for each span
    TranslationOptionCollection::const_iterator iterTransOpt;
    for(iterTransOpt = begin() ; iterTransOpt != end() ; ++iterTransOpt) {
      const TranslationOption &transOpt = *iterTransOpt;
      size_t startpos = transOpt.GetStartPos();
      size_t endpos = transOpt.GetEndPos();
      float score = transOpt.GetFutureScore();
      if (score > m_futureScore.GetScore(startpos, endpos))
        m_futureScore.SetScore(startpos, endpos, score);

      if (printCounts == true) counts[startpos*size + endpos] ++;
    }

    // now fill all the cells in the strictly upper triangle
    //   there is no way to modify the diagonal now, in the case
    //   where no translation option covers a single-word span,
    //   we leave the +inf in the matrix
    // like in chart parsing we want each cell to contain the highest score
    // of the full-span trOpt or the sum of scores of joining two smaller spans

	for(size_t colstart = 1; colstart < size ; colstart++) {
		for(size_t diagshift = 0; diagshift < size-colstart ; diagshift++) {
            size_t startPos = diagshift;
            size_t endPos = colstart+diagshift;
			for(size_t joinAt = startPos; joinAt < endPos ; joinAt++)  {
              float joinedScore = m_futureScore.GetScore(startPos, joinAt)
                                + m_futureScore.GetScore(joinAt+1, endPos);
              /* // uncomment to see the cell filling scheme
              cerr << "[" <<startPos<<","<<endPos<<"] <-? ["<<startPos<<","<<joinAt<<"]+["<<joinAt+1<<","<<endPos
                << "] (colstart: "<<colstart<<", diagshift: "<<diagshift<<")"<<endl;
              */
              if (joinedScore > m_futureScore.GetScore(startPos, endPos))
                m_futureScore.SetScore(startPos, endPos, joinedScore);
            }
        }
    }

    if (printCounts == true) {
      int total = 0;
      for(size_t row=0; row<size; row++)
        for(size_t col=row; col<size; col++)
          if (counts[row*size+ col] > 0) {
	        cout<<"translation options spanning from  "<< row <<" to "<< col <<" is "<< counts[row*size+ col] <<endl;
            total += counts[row*size+ col];
          }
      cout << "translation options generated in total: "<< total << endl;
      free(counts);
	}

	if(verboseLevel > 0) 
	{		
      for(size_t row=0; row<size; row++)
        for(size_t col=row; col<size; col++)
		  cout<<"future cost from "<< row <<" to "<< col <<" is "<< m_futureScore.GetScore(row, col) <<endl;
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



void TranslationOptionCollection::ProcessOneUnknownWord(const FactorArray &sourceWord,
																														size_t sourcePos
																														, int dropUnknown
																														, FactorCollection &factorCollection
																														, float weightWordPenalty)
{
	// unknown word, add to target, and add as poss trans
	//				float	weightWP		= m_staticData.GetWeightWordPenalty();

	//	const FactorArray &sourceWord = m_source.GetFactorArray(sourcePos);

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
		Add(*transOpt);
		delete transOpt;

		m_unknownWordPos.SetValue(sourcePos, true); 
}



void TranslationOptionCollection::ProcessInitialTranslation(
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

		for (size_t endPos = startPos ; endPos < m_source.GetSize() ; endPos++)
		{
			const WordsRange wordsRange(startPos, endPos);
			const TargetPhraseCollection *phraseColl =	phraseDictionary.GetTargetPhraseCollection(m_source,wordsRange); 
			if (phraseColl != NULL)
			{
				if (verboseLevel >= 3) 
				{
					cout << "[" << m_source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n";
				}
				
				TargetPhraseCollection::const_iterator iterTargetPhrase;
				for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != phraseColl->end() ; ++iterTargetPhrase)
				{
					const TargetPhrase	&targetPhrase = *iterTargetPhrase;
							
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
			else if (wordsRange.GetWordsCount() == 1)
			{
				ProcessUnknownWord(startPos, dropUnknown, factorCollection, weightWordPenalty);
				continue;
			}
		}
	}
}

TO_STRING_BODY(TranslationOptionCollection);

