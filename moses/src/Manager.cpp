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

#include <limits>
#include <math.h>
#include "Manager.h"
#include "TypeDef.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "HypothesisCollectionIntermediate.h"
#include "LatticePath.h"
#include "LatticePathCollection.h"

using namespace std;

Manager::Manager(const Sentence &sentence, StaticData &staticData)
:m_source(sentence)
,m_hypoStack(sentence.GetSize() + 1)
,m_staticData(staticData)
,m_futureScore(sentence.GetSize())
{	
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.SetBeamThreshold(m_staticData.GetBeamThreshold());
	}
}

Manager::~Manager()
{
}

void Manager::ProcessSentence()
{
	list < DecodeStep > &decodeStepList = m_staticData.GetDecodeStepList();

	const PhraseDictionary &phraseDictionary = decodeStepList.front().GetPhraseDictionary();
	const LMList &lmListInitial = m_staticData.GetLanguageModel(Initial);
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	CreatePossibleTranslations(m_source, phraseDictionary, lmListInitial);


	// output
	//TRACE_ERR (m_possibleTranslations << endl);


	// seed hypothesis
	{
	Hypothesis *hypo = new Hypothesis(m_source);
#ifdef N_BEST
	LMList allLM = m_staticData.GetAllLM();
	hypo->ResizeComponentScore(allLM, decodeStepList);
#endif
	m_hypoStack[0].Add(hypo, m_staticData.GetBeamThreshold());
	}

	// go thru each stack
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.PruneToSize(MAXIMUM_HYPO_COLL_SIZE/2);
		sourceHypoColl.InitializeArcs();
		//sourceHypoColl.Prune();
		ProcessOneStack(decodeStepList, sourceHypoColl);

		//OutputHypoStack();
		OutputHypoStackSize();
	}

	// output
	//OutputHypoStack();
	OutputHypoStackSize();
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	// best
	const HypothesisCollection &hypoColl = m_hypoStack.back();
	return hypoColl.GetBestHypothesis();
}

void Manager::ProcessOneStack(const list < DecodeStep > &decodeStepList
															,HypothesisCollection &sourceHypoColl)
{
	// go thru each hypothesis in the stack
	HypothesisCollection::iterator iterHypo;
	for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
	{
		Hypothesis &hypothesis = **iterHypo;
		ProcessOneHypothesis(decodeStepList, hypothesis);
	}
}

void Manager::ProcessOneHypothesis(const list < DecodeStep > &decodeStepList
																	 , const Hypothesis &hypothesis)
{
	vector < HypothesisCollectionIntermediate > outputHypoCollVec( decodeStepList.size() );
	HypothesisCollectionIntermediate::iterator iterHypo;

	// initial translation step
	list < DecodeStep >::const_iterator iterStep = decodeStepList.begin();
	const DecodeStep &decodeStep = *iterStep;
	ProcessInitialTranslation(hypothesis, decodeStep, outputHypoCollVec[0]);

	// do rest of decode steps
	int indexStep = 0;
	for (++iterStep ; iterStep != decodeStepList.end() ; ++iterStep) 
	{
		const DecodeStep &decodeStep = *iterStep;
		HypothesisCollectionIntermediate 
								&inputHypoColl		= outputHypoCollVec[indexStep]
								,&outputHypoColl	= outputHypoCollVec[indexStep + 1];

		// is it translation or generation
		switch (decodeStep.GetDecodeType()) 
		{
		case Translate:
			{
				// go thru each hypothesis just created
				for (iterHypo = inputHypoColl.begin() ; iterHypo != inputHypoColl.end() ; ++iterHypo)
				{
					Hypothesis &inputHypo = **iterHypo;
					ProcessTranslation(inputHypo, decodeStep, outputHypoColl);
				}
				break;
			}
		case Generate:
			{
				// go thru each hypothesis just created
				for (iterHypo = inputHypoColl.begin() ; iterHypo != inputHypoColl.end() ; ++iterHypo)
				{
					Hypothesis &inputHypo = **iterHypo;
					ProcessGeneration(inputHypo, decodeStep, outputHypoColl);
				}
				break;
			}
		}

		indexStep++;
	} // for (++iterStep 

	// add to real hypothesis stack
	HypothesisCollectionIntermediate &lastHypoColl	= outputHypoCollVec[decodeStepList.size() - 1];
	for (iterHypo = lastHypoColl.begin() ; iterHypo != lastHypoColl.end() ; )
	{
		Hypothesis *hypo = *iterHypo;

		hypo->CalcScore(m_staticData.GetLanguageModel(Initial)
									, m_staticData.GetLanguageModel(Other)
									, m_staticData.GetWeightDistortion()
									, m_staticData.GetWeightWordPenalty()
									, m_futureScore);
		size_t wordsTranslated = hypo->GetWordsBitmap().GetWordsCount();

		if (m_hypoStack[wordsTranslated].Add(hypo, m_staticData.GetBeamThreshold()))
		{
			HypothesisCollectionIntermediate::iterator iterCurr = iterHypo++;
			lastHypoColl.Detach(iterCurr);
		}
		else
		{
			++iterHypo;
		}
	}
}

void Manager::ProcessInitialTranslation(const Hypothesis &hypothesis
																				, const DecodeStep &decodeStep
																				,HypothesisCollectionIntermediate &outputHypoColl)
{
	int maxDistortion = m_staticData.GetMaxDistortion();
	if (maxDistortion < 0)
	{	// no limit on distortion
		PossibleTranslationCollection::const_iterator iterPossTrans;
		for (iterPossTrans = m_possibleTranslations.begin(); iterPossTrans != m_possibleTranslations.end(); ++iterPossTrans)
		{
			const PossibleTranslation &possTrans = *iterPossTrans;

			if ( !possTrans.Overlap(hypothesis)) 
			{
				Hypothesis *newHypo = hypothesis.CreateNext(possTrans);
				outputHypoColl.AddNoPrune( newHypo );			
			}
		}
	}
	else
	{
		const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
		size_t hypoWordCount		= hypoBitmap.GetWordsCount()
			,hypoFirstGapPos	= hypoBitmap.GetFirstGapPos();

		// MAIN LOOP. go through each possible hypo
		PossibleTranslationCollection::const_iterator iterPossTrans;
		for (iterPossTrans = m_possibleTranslations.begin(); iterPossTrans != m_possibleTranslations.end(); ++iterPossTrans)
		{
			const PossibleTranslation &possTrans = *iterPossTrans;
			// calc distortion if using this poss trans

			size_t possTransStartPos = possTrans.GetStartPos();

			if (hypoFirstGapPos == hypoWordCount)
			{
				if (possTransStartPos == hypoWordCount
					|| (possTransStartPos > hypoWordCount 
					&& possTrans.GetEndPos() <= hypoWordCount + m_staticData.GetMaxDistortion())
					)
				{
					Hypothesis *newHypo = hypothesis.CreateNext(possTrans);
					outputHypoColl.AddNoPrune( newHypo );			
				}
			}
			else
			{
				if (possTransStartPos < hypoWordCount)
				{
					if (possTransStartPos >= hypoFirstGapPos
						&& !possTrans.Overlap(hypothesis))
					{
						Hypothesis *newHypo = hypothesis.CreateNext(possTrans);
						outputHypoColl.AddNoPrune( newHypo );			
					}
				}
				else
				{
					if (possTrans.GetEndPos() <= hypoFirstGapPos + m_staticData.GetMaxDistortion()
						&& !possTrans.Overlap(hypothesis))
					{
						Hypothesis *newHypo = hypothesis.CreateNext(possTrans);
						outputHypoColl.AddNoPrune( newHypo );			
					}
				}
			}
		}
	}
}

void Manager::ProcessTranslation(const Hypothesis &hypothesis
																 , const DecodeStep &decodeStep
																 ,HypothesisCollectionIntermediate &outputHypoColl)
{
	const WordsRange &sourceWordsRange				= hypothesis.GetCurrSourceWordsRange();
	const Phrase sourcePhrase 								= m_source.GetSubString(sourceWordsRange);
	const PhraseDictionary &phraseDictionary	= decodeStep.GetPhraseDictionary();
	const TargetPhraseCollection *phraseColl	=	phraseDictionary.FindEquivPhrase(sourcePhrase);
	size_t currTargetLength										= hypothesis.GetCurrTargetLength();
	
	if (phraseColl != NULL)
	{
		TargetPhraseCollection::const_iterator iterTargetPhrase;

		for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != phraseColl->end(); ++iterTargetPhrase)
		{
			const TargetPhrase &targetPhrase	= *iterTargetPhrase;
			
			float			transScore		= targetPhrase.GetScore()
								,weightWP		= m_staticData.GetWeightWordPenalty();
			PossibleTranslation possTrans(sourceWordsRange
																	, targetPhrase
																	, transScore
																	, weightWP);
			
			Hypothesis *newHypo = hypothesis.MergeNext(possTrans);
			if (newHypo != NULL)
			{
				outputHypoColl.AddNoPrune( newHypo );
			}
		}
	}
	else if (sourceWordsRange.GetWordsCount() == 1 && currTargetLength == 1)
	{ // unknown handler here
		const FactorTypeSet &targetFactors 		= phraseDictionary.GetFactorsUsed(Output);
		Hypothesis *newHypo = new Hypothesis(hypothesis);
		
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			if (targetFactors.Contains(currFactor))
			{
				FactorType factorType = static_cast<FactorType>(currFactor);
				const Factor *targetFactor = newHypo->GetFactor(newHypo->GetSize() - 1, factorType);

				if (targetFactor == NULL)
				{
					const Factor *sourceFactor = sourcePhrase.GetFactor(0, factorType)
											,*unkownfactor;
					switch (factorType)
					{
					case POS:
						unkownfactor = m_staticData.GetFactorCollection().AddFactor(Output, factorType, UNKNOWN_FACTOR);
						newHypo->SetFactor(0, factorType, unkownfactor);
						break;
					default:
						unkownfactor = m_staticData.GetFactorCollection().AddFactor(Output, factorType, sourceFactor->GetString());
						newHypo->SetFactor(0, factorType, unkownfactor);
						break;
					}
				}
			}
		}
		outputHypoColl.AddNoPrune( newHypo );
	}
}

void Manager::CreatePossibleTranslations(const Phrase &phrase
																				 , const PhraseDictionary &phraseDictionary
																				 , const LMList &lmListInitial)
{	
	for (size_t startPos = 0 ; startPos < phrase.GetSize() ; startPos++)
	{
		// reuse phrase, add next word on
		Phrase sourcePhrase( phrase.GetDirection());

		for (size_t endPos = startPos ; endPos < phrase.GetSize() ; endPos++)
		{
			const WordsRange wordsRange(startPos, endPos);

			FactorArray &newWord = sourcePhrase.AddWord();
			Word::Copy(newWord, phrase.GetFactorArray(endPos));

			const TargetPhraseCollection *phraseColl =	phraseDictionary.FindEquivPhrase(sourcePhrase);
			if (phraseColl != NULL)
			{
				TargetPhraseCollection::const_iterator iterTargetPhrase;
				for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != phraseColl->end() ; ++iterTargetPhrase)
				{
					const TargetPhrase	&targetPhrase = *iterTargetPhrase;
					
					float				score				= targetPhrase.GetScore()
											,weightWP		= m_staticData.GetWeightWordPenalty();
					const WordsRange wordsRange(startPos, endPos);
					PossibleTranslation possTrans(wordsRange
																		, targetPhrase
																		, score
																		, lmListInitial
																		, weightWP);
					m_possibleTranslations.push_back(possTrans);
				}
			}
			else if (sourcePhrase.GetSize() == 1)
			{
				// unknown word, add to target, and add as poss trans
				float	weightWP		= m_staticData.GetWeightWordPenalty();
				const FactorTypeSet &targetFactors 		= phraseDictionary.GetFactorsUsed(Output);
				
				// make sure new phrase isn't deallocated while we're using it
				m_unknownPhrase.push_back(TargetPhrase(Output, &phraseDictionary));
				TargetPhrase &targetPhrase = m_unknownPhrase.back();
				FactorArray &targetWord = targetPhrase.AddWord();

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
							unkownfactor = m_staticData.GetFactorCollection().AddFactor(Output, factorType, UNKNOWN_FACTOR);
							targetWord[factorType] = unkownfactor;
							break;
						default:
							unkownfactor = m_staticData.GetFactorCollection().AddFactor(Output, factorType, factor->GetString());
							targetWord[factorType] = unkownfactor;
							break;
						}
					}
				}

				targetPhrase.ResetScore();

				PossibleTranslation possTrans(wordsRange
																		, targetPhrase
																		, 0
																		, lmListInitial
																		, weightWP);
				m_possibleTranslations.push_back(possTrans);
			}
		}
	}

	// create future score matrix
	for(size_t start = 0; start < phrase.GetSize() ; start++) 
	{
		for(size_t end = start; end < phrase.GetSize() ; end++) 
		{
			size_t length = end - start + 1;
			vector< float > score(length + 1);
			score[0] = 0;
			for(size_t currLength = 1 ; currLength <= length ; currLength++) 
			{
				score[currLength] = - numeric_limits<float>::infinity();
			}

			for(size_t currLength = 0 ; currLength < length ; currLength++) 
			{
				PossibleTranslationCollection::const_iterator iterPossTrans;
				for(iterPossTrans = m_possibleTranslations.begin() ; iterPossTrans != m_possibleTranslations.end() ; ++iterPossTrans)
				{
					const PossibleTranslation &possTrans = *iterPossTrans;
					size_t index = currLength + possTrans.GetSize();

					if (possTrans.GetStartPos() == currLength + start 
						&& possTrans.GetEndPos() <= end 
						&& possTrans.GetFutureScore() + score[currLength] > score[index]) 
					{
						score[index] = possTrans.GetFutureScore() + score[currLength];
					}
				}
			}
			m_futureScore.SetScore(start, end, score[length]);
		}
	}
}

// helpers
typedef pair<Word, float> WordPair;
typedef list< WordPair > WordList;	
	// 1st = word 
	// 2nd = score
typedef list< WordPair >::const_iterator WordListIterator;

void IncrementIterators(vector< WordListIterator > &wordListIterVector
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

void Manager::ProcessGeneration(const Hypothesis &hypothesis
																, const DecodeStep &decodeStep
																, HypothesisCollectionIntermediate &outputHypoColl)
{
	const GenerationDictionary &generationDictionary = decodeStep.GetGenerationDictionary();
	size_t idDict = generationDictionary.GetId();
	const float weight = generationDictionary.GetWeight();

	size_t hypoSize	= hypothesis.GetSize()
		, targetLength	= hypothesis.GetCurrTargetLength();

	// generation list for each word in hypothesis
	vector< WordList > wordListVector(targetLength);

	// create generation list
	int wordListVectorPos = 0;
	for (size_t currPos = hypoSize - targetLength ; currPos < hypoSize ; currPos++)
	{
		WordList &wordList = wordListVector[wordListVectorPos];
		const FactorArray &factorArray = hypothesis.GetFactorArray(currPos);

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

		// merge with existing hypothesis
		Hypothesis *mergeHypo = hypothesis.Clone();
		mergeHypo->MergeFactors(mergeWords, idDict, generationScore, weight);
		outputHypoColl.AddNoPrune(mergeHypo);

		// increment iterators
		IncrementIterators(wordListIterVector, wordListVector);
	}
}

void Manager::OutputHypoStackSize()
{
	std::vector < HypothesisCollection >::const_iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		TRACE_ERR ( (int)(*iterStack).size() << ", ");
	}
	TRACE_ERR (endl);
}

void Manager::OutputHypoStack(int stack)
{
	if (stack >= 0)
	{
		TRACE_ERR ( "Stack " << stack << ": " << endl << m_hypoStack[stack] << endl);
	}
	else
	{ // all stacks
		int i = 0;
		vector < HypothesisCollection >::iterator iterStack;
		for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
		{
			HypothesisCollection &hypoColl = *iterStack;
			TRACE_ERR ( "Stack " << i++ << ": " << endl << hypoColl << endl);
		}
	}
}

void Manager::CalcNBest(size_t count, LatticePathList &ret) const
{
#ifdef N_BEST
	if (count <= 0)
		return;

	list<const Hypothesis*> sortedPureHypo = m_hypoStack.back().GetSortedList();

	if (sortedPureHypo.size() == 0)
		return;

	LatticePathCollection contenders;

	// path of the best
	contenders.insert(new LatticePath(*sortedPureHypo.begin()));
	
	// used to add next pure hypo path
	list<const Hypothesis*>::const_iterator iterBestHypo = ++sortedPureHypo.begin();

	for (size_t currBest = 0 ; currBest <= count && contenders.size() > 0 ; currBest++)
	{
		// get next best from list of contenders
		LatticePath *path = *contenders.begin();
		ret.push_back(path);
		contenders.erase(contenders.begin());

		// create deviations from current best
		path->CreateDeviantPaths(contenders);
		
		// if necessary, add next pure path
		if (path->IsPurePath() && iterBestHypo != sortedPureHypo.end())
		{
			contenders.insert(new LatticePath(*iterBestHypo));
			++iterBestHypo;
		}
	}
#endif
}
