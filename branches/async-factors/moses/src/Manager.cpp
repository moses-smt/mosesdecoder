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
#include "LatticePath.h"
#include "LatticePathCollection.h"
#include "TranslationOption.h"
#include "LMList.h"
#include "TranslationOptionCollection.h"

using namespace std;

Manager::Manager(InputType const& source, StaticData &staticData)
:m_source(source)
 ,m_hypoStack(source.GetSize() + 1)
,m_staticData(staticData)
,m_possibleTranslations(*source.CreateTranslationOptionCollection())
,m_initialTargetPhrase(Output)
{
	int pts = m_staticData.GetPhraseDictionaries().size();

	// Need to allocate space for additional collections
	if (m_staticData.GetInputType() == 0)
		{
			m_secondaryOptions = (TranslationOptionCollection **) malloc(sizeof(TranslationOptionCollection *) * pts);
			for (int c = 0; c < pts; c++)
				m_secondaryOptions[c] = source.CreateTranslationOptionCollection();
		}
	m_scoredLMs = 0x0;
	assert(m_staticData.GetAllLM().size() < sizeof(int));

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
	if (m_staticData.GetInputType() == 0)
		{
			for (unsigned int c = 0; c < m_staticData.GetPhraseDictionaries().size(); c++)
				delete m_secondaryOptions[c];
			free(m_secondaryOptions);
		}
	// Clear Hyps before the collections go away
	m_hypoStack[0].FreeHypPool();

  delete &m_possibleTranslations;
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void Manager::ProcessSentence()
{
	m_staticData.ResetSentenceStats(m_source);
	list < DecodeStep* > &decodeStepList = m_staticData.GetDecodeStepList();
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	//m_possibleTranslations.CreateTranslationOptions(decodeStepList
	//, m_staticData.GetFactorCollection());

	list < DecodeStep* >::const_iterator iterStep = decodeStepList.begin();

	list<DecodeStep *> b;
	assert((*iterStep)->GetType() == 0);
	b.push_back(*iterStep); // better not be a generation step!
	m_possibleTranslations.CreateTranslationOptions(b, m_staticData.GetFactorCollection());
	
	//
	// Create Secondary options
	//

	//for (int c = 1; c < m_staticData.GetPhraseDictionaries().size(); c++)
	unsigned int c;
	for (c = 0, ++iterStep ; iterStep != decodeStepList.end() ; ++iterStep) 
		if ((*iterStep)->GetType() == 0)
		{
			list<DecodeStep *> b;
			b.push_back(*iterStep);
			m_secondaryOptions[c++]->CreateTranslationOptions(b, m_staticData.GetFactorCollection());
		}

	// initial seed hypothesis: nothing translated, no words produced
	{
		Hypothesis *hypo = Hypothesis::Create(m_source, m_initialTargetPhrase, -1); // initial PTID
		m_hypoStack[0].AddPrune(hypo);
	}
	
	// go through each stack
	std::vector < HypothesisCollection >::iterator iterStack;

	//c = 0;
	int pt = 1;

	// Process first PT as if it were normal (this is used to compute distortion)
	// This should be identical
	for (iterStack = m_hypoStack.begin(); iterStack != m_hypoStack.end(); ++iterStack)
		{
			HypothesisCollection &sourceHypoColl = *iterStack;
			
			// the stack is pruned before processing (lazy pruning):
			sourceHypoColl.PruneToSize(m_staticData.GetMaxHypoStackSize());
			
			sourceHypoColl.InitializeArcs();
			
			// go through each hypothesis on the stack and try to expand it
			HypothesisCollection::const_iterator iterHypo;
			for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
				{
					Hypothesis &hypothesis = **iterHypo;
					cerr << "Processing Hypo " << hypothesis << endl;
					ProcessOneHypothesis(hypothesis, &m_possibleTranslations, -1); // expand the hypothesis
				}
			// some logging
			if (m_staticData.GetVerboseLevel() > 0) {
				//OutputHypoStack();
				OutputHypoStackSize();
			}
		}

	if (m_staticData.GetInputType()) return;
		
	// Process remaining steps
	int gt = 0, id = 1;
	iterStep = decodeStepList.begin();
	for (++iterStep; iterStep != decodeStepList.end(); ++iterStep)
		{
			if ((*iterStep)->GetType() == 0)
				{
					int i = 0;
					int firsthyp = 1;
					// Create new start(s)
					HypothesisCollection::const_iterator iterHypo;
					HypothesisCollection &currHypoColl = m_hypoStack.back();
					
					// clear stack 0
					m_hypoStack[0].Reset();

					for (iterHypo = currHypoColl.begin() ; iterHypo != currHypoColl.end() ; ++iterHypo, i++)
						{
							Hypothesis &hypothesis = **iterHypo;
							if (firsthyp)
								{
									m_scoredLMs |= hypothesis.GetScoredLMs();
									hypothesis.GetScoredLMs() = 0x0;
									
									firsthyp = 0;
								}
							hypothesis.GetMaskedLMs() = m_scoredLMs;
							cerr << "\t[Transfer Step] Hypo " << i << ": " << hypothesis << endl;
							Hypothesis *restartHypo = new Hypothesis(hypothesis, id);
							cerr << "\t\t* [transfer result] " << *restartHypo << endl;
							m_hypoStack[0].AddPrune(restartHypo);
						}
					// clear remaining stacks
					for (iterStack = m_hypoStack.begin()+1; iterStack != m_hypoStack.end(); ++iterStack)
						(*iterStack).Reset();

					// Now decode the current PT
					fprintf(stderr, "Starting PT processing for %d, %d items in starting stack\n", pt, m_hypoStack[0].size());
					for (iterStack = m_hypoStack.begin(); iterStack != m_hypoStack.end(); ++iterStack)
						{
							HypothesisCollection &sourceHypoColl = *iterStack;
							
							// the stack is pruned before processing (lazy pruning):
							sourceHypoColl.PruneToSize(m_staticData.GetMaxHypoStackSize());
							
							sourceHypoColl.InitializeArcs();
							
							// go through each hypothesis on the stack and try to expand it
							HypothesisCollection::const_iterator iterHypo;
							for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
								{
									Hypothesis &hypothesis = **iterHypo;
									cerr << "\t[PT Processing] Hypo " << i << ": " << hypothesis << endl;
									ProcessOneHypothesis(hypothesis, m_secondaryOptions[pt-1]); // expand the hypothesis
								}
							// some logging
							if (m_staticData.GetVerboseLevel() > 0) {
								//OutputHypoStack();
								OutputHypoStackSize();
							}
						}
					pt++;
				}
			else // do a generation step
				{
					int firsthyp = 1;
					// NOTE: This should be done elsewhere, like a GenerationDecodeStep
					//       but that code has been mostly rewritten to handle TranslationOptions
					//       so this hack is easier for now...

					// normal generation step
					HypothesisCollection::const_iterator iterHypo;
					HypothesisCollection &currHypoColl = m_hypoStack.back();
					HypothesisCollection tmp;
					
					for (iterHypo = currHypoColl.begin() ; iterHypo != currHypoColl.end() ; ++iterHypo)
						{
							Hypothesis &hypothesis = **iterHypo;
							if (firsthyp)
								{
									m_scoredLMs |= hypothesis.GetScoredLMs();
									hypothesis.GetScoredLMs() = 0x0;
									
									firsthyp = 0;
								}
							hypothesis.GetMaskedLMs() = m_scoredLMs;
							cerr << "Generating from Hypo: " << hypothesis << endl;

							// Actual Generation
							const Phrase &targetPhrase  = hypothesis.GetTargetPhrase();
							size_t targetLength = targetPhrase.GetSize();
							vector< WordList > wordListVector(targetLength);

							(*iterStep)->GenerateOptions(wordListVector, targetPhrase);

							// use generation list (wordList)
							// set up iterators (total number of expansions)
							size_t numIteration = 1;
							vector< WordListIterator >  wordListIterVector(targetLength);
							vector< const Word* >       mergeWords(targetLength);
							for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
								{
									wordListIterVector[currPos] = wordListVector[currPos].begin();
									numIteration *= wordListVector[currPos].size();
								}
							//fprintf(stderr, "INFO: numiteration == %d\n", numIteration);
							
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
									//TranslationOption *newTransOpt = MergeGeneration(inputPartialTranslOpt, genPhrase, generationScore);
									//cerr << "INFO: " << genPhrase << std::endl;
									Hypothesis *newh = new Hypothesis(hypothesis, genPhrase, generationScore, id);
									if (newh != NULL)
										{
											//outputPartialTranslOptColl.Add(newTransOpt);
											cerr << "\t\t+ Generating: " << *newh << std::endl;
											tmp.insert(iterHypo, *newh);
										}
									
									// increment iterators
									for (size_t currPos = 0 ; currPos < wordListVector.size() ; currPos++)
										{
											WordListIterator &iter = wordListIterVector[currPos];
											iter++;
											if (iter != wordListVector[currPos].end())
												{ // eg. 4 -> 5
												}
											else
												{ //  eg 9 -> 10
													iter = wordListVector[currPos].begin();
												}
										}
								}
						}
					currHypoColl.Reset();
					currHypoColl.insertset(tmp);
					fprintf(stderr, "After procssing generation step #%d, %d items in hyp collection\n", gt, currHypoColl.size()); 
					gt++;
				}
			id++;
		}
	
	// some more logging
	if (m_staticData.GetVerboseLevel() > 0) {
	  cerr << m_staticData.GetSentenceStats();
    cerr << "Hypotheses created since startup: "<< Hypothesis::s_HypothesesCreated<<endl;
		//OutputHypoStack();
		//OutputHypoStackSize();
	}
}

/** Find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits. 
 * \param hypothesis hypothesis to be expanded upon
 */
void Manager::ProcessOneHypothesis(const Hypothesis &hypothesis,
																	 TranslationOptionCollection *options,
																	 int index)
{
	// since we check for reordering limits, its good to have that limit handy

	// Changed this, secondary processing wants monotone decoding
	int maxDistortion =  m_staticData.GetMaxDistortion(); //  : 0;

	// no limit of reordering: only check for overlap
	if (maxDistortion < 0)
	{	
		const WordsBitmap hypoBitmap	= hypothesis.GetWordsBitmap();
		const size_t hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
								, sourceSize			= m_source.GetSize();

		for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < sourceSize ; ++endPos)
			{
				if (!hypoBitmap.Overlap(WordsRange(startPos, endPos)))
				{
					ExpandAllHypotheses(hypothesis
															, options->GetTranslationOptionList(WordsRange(startPos, endPos))
															, index
															);
				}
			}
		}

		return; // done with special case (no reordering limit)
	}

	// if there are reordering limits, make sure it is not violated
	// the coverage bitmap is handy here (and the position of the first gap)
	const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
	const size_t hypoWordCount		= hypoBitmap.GetNumWordsCovered()
							, hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
							, sourceSize			= m_source.GetSize();
	
	// MAIN LOOP. go through each possible hypo
	for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < sourceSize ; ++endPos)
		{
			// no gap so far => don't skip more than allowed limit
			if (hypoFirstGapPos == hypoWordCount)
				{
					if (startPos == hypoWordCount
							|| (startPos > hypoWordCount 
									&& endPos <= hypoWordCount + maxDistortion)
					)
				{
					ExpandAllHypotheses(hypothesis
															, options->GetTranslationOptionList(WordsRange(startPos, endPos))
															, index
															);
				}
			}
			// filling in gap => just check for overlap
			else if (startPos < hypoWordCount)
				{
					if (startPos >= hypoFirstGapPos
						&& !hypoBitmap.Overlap(WordsRange(startPos, endPos)))
					{
						ExpandAllHypotheses(hypothesis
																, options->GetTranslationOptionList(WordsRange(startPos, endPos))
																, index
																);
					}
				}
			// ignoring, continuing forward => be limited by start of gap
			else
				{
					if (endPos <= hypoFirstGapPos + maxDistortion
						&& !hypoBitmap.Overlap(WordsRange(startPos, endPos)))
					{
						ExpandAllHypotheses(hypothesis
																, options->GetTranslationOptionList(WordsRange(startPos, endPos))
																, index
																);
					}
				}
		}
	}
}

/**
 * Expand a hypothesis given a list of translation options
 * \param hypothesis hypothesis to be expanded upon
 * \param transOptList list of translation options to be applied
 */

void Manager::ExpandAllHypotheses(const Hypothesis &hypothesis,const TranslationOptionList &transOptList, int index)
{
	TranslationOptionList::const_iterator iter;
	for (iter = transOptList.begin() ; iter != transOptList.end() ; ++iter)
	{
		ExpandHypothesis(hypothesis, **iter, index);
	}
}

/**
 * Expand one hypothesis with a translation option.
 * this involves initial creation, scoring and adding it to the proper stack
 * \param hypothesis hypothesis to be expanded upon
 * \param transOpt translation option (phrase translation) 
 *        that is applied to create the new hypothesis
 */
void Manager::ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt, int index) 
{
	cerr << "Trying to expand: " << hypothesis << " with " << transOpt << std::endl;
	if (index > -1 && 
			(transOpt.GetTargetPhrase().GetSize() + hypothesis.GetTargetLen() > hypothesis.GetTargetPhrase().GetSize()
			 || !hypothesis.GetTargetPhrase().IsCompatiblePartial(transOpt.GetTargetPhrase(), hypothesis.GetTargetLen())
			 )
			)
			return;
	// create hypothesis and calculate all its scores
	Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
	newHypo->CalcScore(m_staticData, 
										 (index == -1 ? m_possibleTranslations.GetFutureScore()
											: m_secondaryOptions[index]->GetFutureScore())
										 );
	
	// logging for the curious
	if(m_staticData.GetVerboseLevel() > 2) 
		{			
			newHypo->PrintHypothesis(m_source, m_staticData.GetWeightDistortion(), m_staticData.GetWeightWordPenalty());
		}
	
	// add to hypothesis stack
	size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();	
	cerr << "\t+ Adding: " << *newHypo << "\n";
	m_hypoStack[wordsTranslated].AddPrune(newHypo);
}

/**
 * Find best hypothesis on the last stack.
 * This is the end point of the best translation, which can be traced back from here
 */
const Hypothesis *Manager::GetBestHypothesis() const
{
	const HypothesisCollection &hypoColl = m_hypoStack.back();
	return hypoColl.GetBestHypothesis();
}

/**
 * Logging of hypothesis stack sizes
 */
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

/**
 * Logging of hypothesis stack contents
 * \param stack number of stack to be reported, report all stacks if 0 
 */
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

/**
 * After decoding, the hypotheses in the stacks and additional arcs
 * form a search graph that can be mined for n-best lists.
 * The heavy lifting is done in the LatticePath and LatticePathCollection
 * this function controls this for one sentence.
 *
 * \param count the number of n-best translations to produce
 * \param ret holds the n-best list that was calculated
 */
void Manager::CalcNBest(size_t count, LatticePathList &ret) const
{
	if (count <= 0)
		return;

	vector<const Hypothesis*> sortedPureHypo = m_hypoStack.back().GetSortedList();

	if (sortedPureHypo.size() == 0)
		return;

	LatticePathCollection contenders;

	// path of the best
	contenders.insert(new LatticePath(*sortedPureHypo.begin()));
	
	// used to add next pure hypo path
	vector<const Hypothesis*>::const_iterator iterBestHypo = ++sortedPureHypo.begin();

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
}

void Manager::CalcDecoderStatistics(const StaticData& staticData) const
{
	staticData.GetSentenceStats().CalcFinalStats(*GetBestHypothesis());
}
