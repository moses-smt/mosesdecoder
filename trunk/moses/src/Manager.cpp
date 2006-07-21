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
#include <cmath>
#include "Manager.h"
#include "TypeDef.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "HypothesisCollectionIntermediate.h"
#include "LatticePath.h"
#include "LatticePathCollection.h"
#include "TranslationOption.h"

using namespace std;

Manager::Manager(InputType const& source, 
								 TranslationOptionCollection& toc,
								 StaticData &staticData)
:m_source(source)
,m_hypoStack(source.GetSize() + 1)
,m_staticData(staticData)
,m_possibleTranslations(toc)  
{
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.SetMaxHypoStackSize(m_staticData.GetMaxHypoStackSize());
		sourceHypoColl.SetBeamThreshold(m_staticData.GetBeamThreshold());
	}
}

Manager::~Manager()
{
}

void Manager::ProcessSentence()
{
	
	list < DecodeStep > &decodeStepList = m_staticData.GetDecodeStepList();
	const LMList &lmListInitial = m_staticData.GetLanguageModel(Initial);
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_possibleTranslations.CreateTranslationOptions(decodeStepList
  														, lmListInitial
  														, m_staticData.GetAllLM()
  														, m_staticData.GetFactorCollection()
  														, m_staticData.GetWeightWordPenalty()
  														, m_staticData.GetDropUnknown()
  														, m_staticData.GetVerboseLevel());

	// output
	//TRACE_ERR (m_possibleTranslations << endl);


	// seed hypothesis
	{
		Hypothesis *hypo = Hypothesis::Create(m_possibleTranslations.GetInitialCoverage());
		TRACE_ERR(m_possibleTranslations.GetInitialCoverage().GetWordsCount() << endl);
#ifdef N_BEST
		LMList allLM = m_staticData.GetAllLM();
		hypo->ResizeComponentScore(allLM, decodeStepList);
#endif
		m_hypoStack[m_possibleTranslations.GetInitialCoverage().GetWordsCount()].AddPrune(hypo);
	}
	
	// go thru each stack
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.PruneToSize(m_staticData.GetMaxHypoStackSize());
		sourceHypoColl.InitializeArcs();
		//sourceHypoColl.Prune();
		ProcessOneStack(decodeStepList, sourceHypoColl);

		//OutputHypoStack();
		if (m_staticData.GetVerboseLevel() > 0) {
			OutputHypoStackSize();
		}

	}

	// output
	//OutputHypoStack();
	//OutputHypoStackSize();
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	// best
	const HypothesisCollection &hypoColl = m_hypoStack.back();
	return hypoColl.GetBestHypothesis();
}

void Manager::ProcessOneStack(const list < DecodeStep > &decodeStepList, HypothesisCollection &sourceHypoColl)
{
	// go thru each hypothesis in the stack
	HypothesisCollection::iterator iterHypo;
	for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
	{
		Hypothesis &hypothesis = **iterHypo;
		ProcessOneHypothesis(decodeStepList, hypothesis);
	}
}

void Manager::ProcessOneHypothesis(const list < DecodeStep > &decodeStepList, const Hypothesis &hypothesis)
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
		case InsertNullFertilityWord:
			{
				for (iterHypo = inputHypoColl.begin() ; iterHypo != inputHypoColl.end() ; ++iterHypo)
				{
					Hypothesis &inputHypo = **iterHypo;
					ProcessFinalNullFertilityInsertion(inputHypo, decodeStep, outputHypoColl);
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

		hypo->CalcScore(m_staticData, m_possibleTranslations.GetFutureScore());
		if(m_staticData.GetVerboseLevel() > 2) 
		{			
			hypo->PrintHypothesis(m_source, m_staticData.GetWeightDistortion(), m_staticData.GetWeightWordPenalty());
		}
		size_t wordsTranslated = hypo->GetWordsBitmap().GetWordsCount();

		if (m_hypoStack[wordsTranslated].AddPrune(hypo))
		{
			HypothesisCollectionIntermediate::iterator iterCurr = iterHypo++;
			lastHypoColl.Detach(iterCurr);
			if(m_staticData.GetVerboseLevel() > 2) 
				{
					if(m_hypoStack[wordsTranslated].getBestScore() == hypo->GetScore(ScoreType::Total))
						{
							cout<<"new best estimate for this stack"<<endl;
							
						}
					cout<<"added hypothesis on stack "<<wordsTranslated<<" now size "<<m_hypoStack[wordsTranslated].size()<<endl<<endl;
				}

		}
		else
		{
			++iterHypo;
		}
	}
}

void Manager::ProcessInitialTranslation(const Hypothesis &hypothesis
																				, const DecodeStep & //decodeStep
																				, HypothesisCollectionIntermediate &outputHypoColl)
{
	const WordsBitmap& hypoBitmap = hypothesis.GetWordsBitmap();
	size_t hypoWordCount		= hypoBitmap.GetWordsCount() 		// pharaoh: foreignTranslated
				,hypoFirstGapPos	= hypoBitmap.GetFirstGapPos();	// pharaoh: gapStart

  // TODO: handle this switch polymorphically or with stl algorithms?
  //       that could make this MUCH cleaner. ask cdyer
	int maxDistortion = m_staticData.GetMaxDistortion();
	if (maxDistortion < 0)
	{	// no limit on distortion
		TranslationOptionCollection::const_iterator iterTransOpt;
		for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = *iterTransOpt;

			if ( !transOpt.Overlap(hypothesis)) 
			{
				if (!transOpt.IsDeletionOption() || transOpt.GetStartPos() == hypoWordCount) {
					Hypothesis* newHypo = hypothesis.CreateNext(transOpt);
					outputHypoColl.AddNoPrune( newHypo );			
				}
			}
		}
	}
	else // limited reordering possible (maxDistortion # of words)
	{
		TranslationOptionCollection::const_iterator iterTransOpt;
		for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = *iterTransOpt;
			// calc distortion if using this poss trans

			size_t transOptStartPos = transOpt.GetStartPos();

			if (!transOpt.IsDeletionOption() || transOptStartPos == hypoWordCount)
			{
				if (hypoFirstGapPos == hypoWordCount) // no gap so far
				{
					if (transOptStartPos == hypoWordCount          // monotone
						|| (transOptStartPos > hypoWordCount         // || skip a few source words, but make sure
						&& transOpt.GetEndPos() <= hypoWordCount + maxDistortion)  // the end of the source phrase isn't too far away
						)
					{
						Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
						outputHypoColl.AddNoPrune( newHypo );			
					}
				}
				else  // there has been a gap
				{
					if (transOptStartPos < hypoWordCount)  // go back and fill in a gap?
					{                                      // yes:
						if (transOptStartPos >= hypoFirstGapPos
							&& !transOpt.Overlap(hypothesis))
						{
							Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
							outputHypoColl.AddNoPrune( newHypo );			
						}
					}
					else                                   // no, don't fill it in yet:
					{
						if (transOpt.GetEndPos() <= hypoFirstGapPos + maxDistortion
							&& !transOpt.Overlap(hypothesis))
						{
							Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
							outputHypoColl.AddNoPrune( newHypo );			
						}
					}
				}
			}
		}
	}
}

void Manager::ProcessTranslation(const Hypothesis &hypothesis, const DecodeStep &decodeStep, HypothesisCollectionIntermediate &outputHypoColl)
{
	size_t currTargetLength										= hypothesis.GetCurrTargetLength();

	// if the initial translation step dropped a word, the target phrase
	// length will be 0.  in this case, secondary translation steps will
	// fail.  see comments in ProcessGeneration
	if (currTargetLength == 0)
	{
		Hypothesis *copyHypo = new Hypothesis(hypothesis);
		outputHypoColl.AddNoPrune(copyHypo);
		return;
	}

	// actual implementation
	const WordsRange &sourceWordsRange				= hypothesis.GetCurrSourceWordsRange();
	const PhraseDictionaryBase &phraseDictionary	= decodeStep.GetPhraseDictionary();
	//	const TargetPhraseCollection *phraseColl	=	CreateTargetPhraseCollection(&phraseDictionary,&m_source,sourceWordsRange); 
	const TargetPhraseCollection *phraseColl	=	m_source.CreateTargetPhraseCollection(phraseDictionary,sourceWordsRange); 

	if (phraseColl != NULL)
	{
		TargetPhraseCollection::const_iterator iterTargetPhrase;

		for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != phraseColl->end(); ++iterTargetPhrase)
		{
			const TargetPhrase& targetPhrase	= *iterTargetPhrase;
			
			TranslationOption transOpt(sourceWordsRange
																	, targetPhrase);
	
			Hypothesis *newHypo = hypothesis.MergeNext(transOpt);
			
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
					const Factor *sourceFactor = m_source.GetFactorArray(sourceWordsRange.GetStartPos())[factorType]
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

#if 0
/***
 * Add to m_possibleTranslations all possible translations the phrase table gives us for
 * the given phrase
 * 
 * \param phrase The source phrase to translate
 * \param phraseDictionary The phrase table
 * \param lmListInitial A list of language models
 */
void Manager::CreateTranslationOptions(const Phrase &phrase, PhraseDictionary &phraseDictionary, const LMList &lmListInitial)
{	
	// loop over all substrings of the source sentence, look them up
	// in the phraseDictionary (which is the- possibly filtered-- phrase
	// table loaded on initialization), generate TranslationOption objects
	// for all phrases
	//
	// possible optimization- don't consider phrases longer than the longest
	// phrase in the PhraseDictionary?
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
//      	if (m_staticData.GetVerboseLevel() >= 3) {
//					cout << "[" << sourcePhrase << "; " << startPos << "-" << endPos << "]\n";
 //     	}
				TargetPhraseCollection::const_iterator iterTargetPhrase;
				for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != phraseColl->end() ; ++iterTargetPhrase)
				{
					const TargetPhrase	&targetPhrase = *iterTargetPhrase;
					
					const WordsRange wordsRange(startPos, endPos);
//					TranslationOption transOpt(wordsRange, targetPhrase);
					m_possibleTranslations.push_back(TranslationOption(wordsRange, targetPhrase));
//      		if (m_staticData.GetVerboseLevel() >= 3) {
//						cout << "\t" << transOpt << "\n";
//	     		}
				}
//        if (m_staticData.GetVerboseLevel() >= 3) { cout << endl; }
			}
			else if (sourcePhrase.GetSize() == 1)
			{
				/*
				 * changed to have an extendable unknown-word translation module -- EVH
				 */
//				boost::shared_ptr<std::list<TranslationOption> > unknownWordTranslations = m_staticData.GetUnknownWordHandler()->GetPossibleTranslations(wordsRange, sourcePhrase, m_staticData, phraseDictionary);
//				m_possibleTranslations.insert(m_possibleTranslations.end(), unknownWordTranslations->begin(), unknownWordTranslations->end());
			}
		}
	}

	// create future score matrix
	// for each span in the source phrase (denoted by start and end)
	for(size_t start = 0; start < phrase.GetSize() ; start++) 
	{
		for(size_t end = start; end < phrase.GetSize() ; end++) 
		{
			size_t length = end - start + 1;
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
				for(iterTransOpt = m_possibleTranslations.begin() ; iterTransOpt != m_possibleTranslations.end() ; ++iterTransOpt)
				{
					const TranslationOption &transOpt = *iterTransOpt;
					size_t index = currLength + transOpt.GetSize();

					if (transOpt.GetStartPos() == currLength + start 
						&& transOpt.GetEndPos() <= end 
						&& transOpt.GetFutureScore() + score[currLength] > score[index]) 
					{
						score[index] = transOpt.GetFutureScore() + score[currLength];
					}
				}
			}
			// record the highest cost option in the future cost table.
//			m_futureScore[start][end] = score[length];
			//m_futureScore.SetScore(start, end, score[length]);

			//print information about future cost table when verbose option is set

			if(m_staticData.GetVerboseLevel() > 2) 
			{		
				cout<<"future cost from "<<start<<" to "<<end<<" is "<<score[length]<<endl;
			}
		}
	}
}
#endif

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
	const float weight = generationDictionary.GetWeight();

	size_t hypoSize	= hypothesis.GetSize()
		, targetLength	= hypothesis.GetCurrTargetLength();

	// if the initial translation step dropped a word, the target phrase
	// length will be 0.  in this case, generation will fail.  however,
	// this is not desirable, so we preserve these hypotheses automatically
	if (targetLength == 0)
	{
		Hypothesis *copyHypo = new Hypothesis(hypothesis);
		// TODO: should there be some sort of extra penalty associated with this?
		// current thinking: no, if there needs to be a higher penalty, MERT will
		// do it
		outputHypoColl.AddNoPrune(copyHypo);
		return;
	}

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
		Hypothesis *mergeHypo = new Hypothesis(hypothesis);
		mergeHypo->MergeFactors(mergeWords, generationDictionary, generationScore, weight);
		outputHypoColl.AddNoPrune(mergeHypo);

		// increment iterators
		IncrementIterators(wordListIterVector, wordListVector);
	}
}

void Manager::ProcessFinalNullFertilityInsertion(const Hypothesis &hypothesis
																				, const DecodeStep & //decodeStep
																				, HypothesisCollectionIntermediate &outputHypoColl)
{
	Hypothesis *copyHypo = new Hypothesis(hypothesis);
	outputHypoColl.AddNoPrune(copyHypo);
}

void Manager::OutputHypoStackSize()
{
	std::vector < HypothesisCollection >::const_iterator iterStack = m_hypoStack.begin();
	TRACE_ERR ((int)iterStack->size());
	for (++iterStack; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		TRACE_ERR ( ", " << (int)iterStack->size());
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
