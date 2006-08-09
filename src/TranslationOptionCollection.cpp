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

#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "Input.h"
#include "Util.h"

#include "StaticData.h"

using namespace std;

/** constructor; since translation options are indexed by coverage span, the corresponding data structure is initialized here */
TranslationOptionCollection::TranslationOptionCollection(InputType const& src, size_t maxNoTransOptPerCoverage)
	: m_source(src)
	,m_futureScore(src.GetSize())
	,m_unknownWordPos(src.GetSize())
	,m_maxNoTransOptPerCoverage(maxNoTransOptPerCoverage)
{
	// create 2-d vector
	size_t size = src.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		m_collection.push_back( vector< TranslationOptionList >() );
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
			m_collection[startPos].push_back( TranslationOptionList() );
		}
	}
}

/** destructor, clears out data structures */
TranslationOptionCollection::~TranslationOptionCollection()
{
	// delete all trans opt
	size_t size = m_source.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
			RemoveAllInColl<TranslationOptionList::iterator>(GetTranslationOptionList(startPos, endPos));
		}
	}
}

/** helper for pruning */
bool CompareTranslationOption(const TranslationOption *a, const TranslationOption *b)
{
	return a->GetFutureScore() > b->GetFutureScore();
}

/** pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
void TranslationOptionCollection::Prune()
{
	if (m_maxNoTransOptPerCoverage == 0)
		return;

	size_t total = 0;
	size_t totalPruned = 0;
	size_t size = m_source.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
			TranslationOptionList &fullList = GetTranslationOptionList(startPos, endPos);
			total += fullList.size();
			if (fullList.size() <= m_maxNoTransOptPerCoverage)
				continue;
			
			// sort in vector
			nth_element(fullList.begin(), fullList.begin() + m_maxNoTransOptPerCoverage, fullList.end(), CompareTranslationOption);

			totalPruned += fullList.size() - m_maxNoTransOptPerCoverage;
			
			// delete the rest
			for (size_t i = m_maxNoTransOptPerCoverage ; i < fullList.size() ; ++i)
			{
				delete fullList[i];
			}
			fullList.resize(m_maxNoTransOptPerCoverage);
		}
	}
	if (StaticData::Instance()->GetVerboseLevel() >= 1)
	{
		std::cerr << "       Total translation options: " << total << std::endl;
		std::cerr << "Total translation options pruned: " << totalPruned << std::endl;
	}
}

/** compute the future score matrix used in search */
void TranslationOptionCollection::CalcFutureScore()
{
	// create future score matrix in a dynamic programming fashion

  // setup the matrix (ignore lower triangle, set upper triangle to -inf
  size_t size = m_source.GetSize(); // the width of the matrix

  for(size_t row=0; row<size; row++) {
    for(size_t col=row; col<size; col++) {
      m_futureScore.SetScore(row, col, -numeric_limits<float>::infinity());
    }
  }

  // walk all the translation options and record the cheapest option for each span
	for (size_t startPos = 0 ; startPos < m_source.GetSize() ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < m_source.GetSize() ; ++endPos)
		{
			TranslationOptionList &transOptList = GetTranslationOptionList(startPos, endPos);

			TranslationOptionList::const_iterator iterTransOpt;
			for(iterTransOpt = transOptList.begin() ; iterTransOpt != transOptList.end() ; ++iterTransOpt) 
			{
				const TranslationOption &transOpt = **iterTransOpt;
				float score = transOpt.GetFutureScore();
				if (score > m_futureScore.GetScore(startPos, endPos))
					m_futureScore.SetScore(startPos, endPos, score);
			}
		}
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

	if(StaticData::Instance()->GetVerboseLevel() >= 3)
	{		
      int total = 0;
      for(size_t row=0; row<size; row++)
      {
        for(size_t col=row; col<size; col++)
        {
        	int count = GetTranslationOptionList(row, col).size();
	        TRACE_ERR("translation options spanning from  "
	        				<< row <<" to "<< col <<" is "
	        				<< count <<endl);
       		total += count;
        }
      }
      TRACE_ERR("translation options generated in total: "<< total << endl);

      for(size_t row=0; row<size; row++)
        for(size_t col=row; col<size; col++)
		  TRACE_ERR("future cost from "<< row <<" to "<< col <<" is "<< m_futureScore.GetScore(row, col) <<endl);
    }
}


// helpers
typedef pair<Word, ScoreComponentCollection2> WordPair;
typedef list< WordPair > WordList;	
// 1st = word 
// 2nd = score
typedef list< WordPair >::const_iterator WordListIterator;

/** used in generation: increases iterators when looping through the exponential number of generation expansions */
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

/** apply generation decoding step to a list of partial translation options */
void TranslationOptionCollection::ProcessGeneration(			
																										const TranslationOption &inputPartialTranslOpt
																										, const DecodeStep &decodeStep
																										, PartialTranslOptColl &outputPartialTranslOptColl
																										, FactorCollection &factorCollection)
{
	//TRACE_ERR(inputPartialTranslOpt << endl);
	if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
		{ // word deletion
		
			TranslationOption *newTransOpt = new TranslationOption(inputPartialTranslOpt);
			outputPartialTranslOptColl.Add(newTransOpt);
		
			return;
		}
	
	// normal generation step
	const GenerationDictionary &generationDictionary	= decodeStep.GetGenerationDictionary();
	const WordsRange &sourceWordsRange								= inputPartialTranslOpt.GetSourceWordsRange();

	const Phrase &targetPhrase	= inputPartialTranslOpt.GetTargetPhrase();
	size_t targetLength					= targetPhrase.GetSize();

	// generation list for each word in phrase
	vector< WordList > wordListVector(targetLength);

	// create generation list
	int wordListVectorPos = 0;
	for (size_t currPos = 0 ; currPos < targetLength ; currPos++) // going thorugh all words
		{
			// generatable factors for this word to be put in wordList
			WordList &wordList = wordListVector[wordListVectorPos];
			const FactorArray &factorArray = targetPhrase.GetFactorArray(currPos);

			// consult dictionary for possible generations for this word
			const OutputWordCollection *wordColl = generationDictionary.FindWord(factorArray);

			if (wordColl == NULL)
				{	// word not found in generation dictionary
					ProcessUnknownWord(sourceWordsRange.GetStartPos(), factorCollection);
					return; // can't be part of a phrase, special handling
				}
			else
				{
					// sort(*wordColl, CompareWordCollScore);
					OutputWordCollection::const_iterator iterWordColl;
					for (iterWordColl = wordColl->begin() ; iterWordColl != wordColl->end(); ++iterWordColl)
						{
							const Word &outputWord = (*iterWordColl).first;
							const ScoreComponentCollection2& score = (*iterWordColl).second;
							// enter into word list generated factor(s) and its(their) score(s)
							wordList.push_back(WordPair(outputWord, score));
						}
		
					wordListVectorPos++; // done, next word
				}
		}

	// use generation list (wordList)
	// set up iterators (total number of expansions)
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
			ScoreComponentCollection2 generationScore; // total score for this string of words

			// create vector of words with new factors for last phrase
			for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
				{
					const WordPair &wordPair = *wordListIterVector[currPos];
					mergeWords[currPos] = &(wordPair.first);
					generationScore.PlusEquals(wordPair.second);
				}

			// merge with existing trans opt
			Phrase genPhrase(Output, mergeWords);
			TranslationOption *newTransOpt = inputPartialTranslOpt.MergeGeneration(genPhrase, generationScore);
			if (newTransOpt != NULL)
				{
					outputPartialTranslOptColl.Add(newTransOpt);
				}

			// increment iterators
			IncrementIterators(wordListIterVector, wordListVector);
		}
}

/** apply translation step to list of partial translation options */
void TranslationOptionCollection::ProcessTranslation(
																										 const TranslationOption &inputPartialTranslOpt
																										 , const DecodeStep		 &decodeStep
																										 , PartialTranslOptColl &outputPartialTranslOptColl
																										 , FactorCollection &factorCollection)
{
	//TRACE_ERR(inputPartialTranslOpt << endl);
	if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
		{ // word deletion
		
			outputPartialTranslOptColl.Add(new TranslationOption(inputPartialTranslOpt));
		
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
							outputPartialTranslOptColl.Add( newTransOpt );
						}
				}
		}
	else if (sourceWordsRange.GetWordsCount() == 1)
		{ // unknown handler
			ProcessUnknownWord(sourceWordsRange.GetStartPos(), factorCollection);
		}
}


/** Collect all possible translations from the phrase tables
 * for a particular input sentence. This implies applying all
 * translation and generation steps. Also computes future cost matrix.
 * \param decodeStepList list of decoding steps
 * \param factorCollection input sentence with all factors
 */
void TranslationOptionCollection::CreateTranslationOptions(
																													 const list < DecodeStep > &decodeStepList
																													 , FactorCollection &factorCollection)
{
	for (size_t startPos = 0 ; startPos < m_source.GetSize() ; startPos++)
		{
		for (size_t endPos = startPos ; endPos < m_source.GetSize() ; endPos++)
		{
			CreateTranslationOptionsForRange( decodeStepList, factorCollection, startPos, endPos);
		}
	}

	// Prune
	Prune();

	// future score matrix
	CalcFutureScore();
}

/** subroutine for CreateTranslationOptions: collect translation options
 * that exactly cover a specific input span
 * \param decodeStepList list of decoding steps
 * \param factorCollection input sentence with all factors
 * \param startPos first position in input sentence
 * \param lastPos last position in input sentence 
 */
void TranslationOptionCollection::CreateTranslationOptionsForRange(
																													 const list < DecodeStep > &decodeStepList
																													 , FactorCollection &factorCollection
																													 , size_t startPos
																													 , size_t endPos)
{
	// partial trans opt stored in here
	vector < PartialTranslOptColl* > outputPartialTranslOptCollVec( decodeStepList.size() );
	outputPartialTranslOptCollVec[0] = new PartialTranslOptColl();
	
	// initial translation step
	list < DecodeStep >::const_iterator iterStep = decodeStepList.begin();
	const DecodeStep &decodeStep = *iterStep;

	ProcessInitialTranslation(decodeStep, factorCollection
														, *outputPartialTranslOptCollVec[0]
														, startPos, endPos );

	// do rest of decode steps
	size_t totalEarlyPruned = 0;
	int indexStep = 0;
	for (++iterStep ; iterStep != decodeStepList.end() ; ++iterStep) 
		{
			const DecodeStep &decodeStep = *iterStep;
			
			outputPartialTranslOptCollVec[indexStep + 1]			= new PartialTranslOptColl();
			PartialTranslOptColl &inputPartialTranslOptColl		= *outputPartialTranslOptCollVec[indexStep]
													,&outputPartialTranslOptColl	= *outputPartialTranslOptCollVec[indexStep + 1];

			// is it translation or generation
			switch (decodeStep.GetDecodeType()) 
				{
				case Translate:
					{
						// go thru each intermediate trans opt just created
						vector<TranslationOption*> partTransOptList = inputPartialTranslOptColl.GetList();
						vector<TranslationOption*>::const_iterator iterPartialTranslOpt;
						for (iterPartialTranslOpt = partTransOptList.begin() ; iterPartialTranslOpt != partTransOptList.end() ; ++iterPartialTranslOpt)
							{
								TranslationOption &inputPartialTranslOpt = **iterPartialTranslOpt;
								ProcessTranslation(inputPartialTranslOpt
																	 , decodeStep
																	 , outputPartialTranslOptColl
																	 , factorCollection);
							}
						break;
					}
				case Generate:
					{
						// go thru each hypothesis just created
						vector<TranslationOption*> partTransOptList = inputPartialTranslOptColl.GetList();
						vector<TranslationOption*>::const_iterator iterPartialTranslOpt;
						for (iterPartialTranslOpt = partTransOptList.begin() ; iterPartialTranslOpt != partTransOptList.end() ; ++iterPartialTranslOpt)
							{
								TranslationOption &inputPartialTranslOpt = **iterPartialTranslOpt;
								ProcessGeneration(inputPartialTranslOpt
																	, decodeStep
																	, outputPartialTranslOptColl
																	, factorCollection);
							}
						break;
					}
				case InsertNullFertilityWord:
					{ // TODO ask chris or evan what should be done
						assert(false);
						break;
					}
				}
			// last but 1 partial trans not required anymore
			totalEarlyPruned += outputPartialTranslOptCollVec[indexStep]->GetPrunedCount();
			delete outputPartialTranslOptCollVec[indexStep];
			indexStep++;
		} // for (++iterStep 

	// add to fully formed translation option list
	PartialTranslOptColl &lastPartialTranslOptColl	= *outputPartialTranslOptCollVec[decodeStepList.size() - 1];
	vector<TranslationOption*> partTransOptList = lastPartialTranslOptColl.GetList();
	vector<TranslationOption*>::iterator iterColl;
	for (iterColl = partTransOptList.begin() ; iterColl != partTransOptList.end() ; ++iterColl)
		{
			TranslationOption *transOpt = *iterColl;
			transOpt->CalcScore();
			Add(transOpt);
		}

	lastPartialTranslOptColl.DetachAll();
	totalEarlyPruned += outputPartialTranslOptCollVec[decodeStepList.size() - 1]->GetPrunedCount();
	delete outputPartialTranslOptCollVec[decodeStepList.size() - 1];
	// cerr << "Early translation options pruned: " << totalEarlyPruned << endl;
}


/** special handling of unknown words: add special translation (or drop) */
void TranslationOptionCollection::ProcessOneUnknownWord(const FactorArray &sourceWord,
																														size_t sourcePos
																												, FactorCollection &factorCollection)
{
	// unknown word, add as trans opt

		size_t isDigit = 0;
		if (StaticData::Instance()->GetDropUnknown())
		{
			const Factor *f = sourceWord[0]; // ??? hack. shouldn't know which factor is surface
			std::string s = f->ToString();
			isDigit = s.find_first_of("0123456789");
			if (isDigit == string::npos) 
				isDigit = 0;
			else 
				isDigit = 1;
			// modify the starting bitmap
		}
		
		TranslationOption *transOpt;
		if (! StaticData::Instance()->GetDropUnknown() || isDigit)
		{
			// add to dictionary
			TargetPhrase targetPhrase(Output);
			FactorArray &targetWord = targetPhrase.AddWord();
						
			for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
			{
				FactorType factorType = static_cast<FactorType>(currFactor);
				
				const Factor *sourceFactor = sourceWord[currFactor];
				if (sourceFactor == NULL)
					targetWord[factorType] = factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR);
				else
					targetWord[factorType] = factorCollection.AddFactor(Output, factorType, sourceFactor->GetString());
			}
	
			targetPhrase.SetScore();
			
			transOpt = new TranslationOption(WordsRange(sourcePos, sourcePos), targetPhrase, 0);
		}
		else 
		{ // drop source word. create blank trans opt
			const TargetPhrase targetPhrase(Output);
			transOpt = new TranslationOption(WordsRange(sourcePos, sourcePos), targetPhrase, 0);
		}

		transOpt->CalcScore();
		Add(transOpt);

		m_unknownWordPos.SetValue(sourcePos, true); 
}


/** initialize list of partial translation options by applying the first translation step */
void TranslationOptionCollection::ProcessInitialTranslation(
															const DecodeStep &decodeStep
															, FactorCollection &factorCollection
															, PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos
															, size_t endPos)
{
	// loop over all substrings of the source sentence, look them up
	// in the phraseDictionary (which is the- possibly filtered-- phrase
	// table loaded on initialization), generate TranslationOption objects
	// for all phrases
	//
	// possible optimization- don't consider phrases longer than the longest
	// phrase in the PhraseDictionary?
	
	const PhraseDictionaryBase &phraseDictionary = decodeStep.GetPhraseDictionary();

	if (m_unknownWordPos.GetValue(startPos))
	{ // unknown word but already processed. skip 
		return;
	}

	const WordsRange wordsRange(startPos, endPos);
	const TargetPhraseCollection *phraseColl =	phraseDictionary.GetTargetPhraseCollection(m_source,wordsRange); 
	if (phraseColl != NULL)
	{
		if (StaticData::Instance()->GetVerboseLevel() >= 3)
		{
			TRACE_ERR("[" << m_source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n");
		}
			
		TargetPhraseCollection::const_iterator iterTargetPhrase;
		for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != phraseColl->end() ; ++iterTargetPhrase)
		{
			const TargetPhrase	&targetPhrase = *iterTargetPhrase;
			outputPartialTranslOptColl.Add ( new TranslationOption(wordsRange, targetPhrase) );
			
			if (StaticData::Instance()->GetVerboseLevel() >= 3)
			{
				TRACE_ERR("\t" << targetPhrase << "\n");
			}
		}
		if (StaticData::Instance()->GetVerboseLevel() >= 3)
		{ 
			TRACE_ERR(endl);
		}
	}
	// handling unknown words
	else if (wordsRange.GetWordsCount() == 1)
	{
		ProcessUnknownWord(startPos, factorCollection);
	}
}

/** add translation option to the list
 * \param translationOption translation option to be added */
void TranslationOptionCollection::Add(const TranslationOption *translationOption)
{
	const WordsRange &coverage = translationOption->GetSourceWordsRange();
	m_collection[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()].push_back(translationOption);
}

TO_STRING_BODY(TranslationOptionCollection);

