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

using namespace std;

Manager::Manager(InputType const& source, 
								 TranslationOptionCollection& toc,
								 StaticData &staticData)
:m_source(source)
,m_hypoStack(source.GetSize() + 1)
,m_staticData(staticData)
,m_possibleTranslations(toc)  //dynamic_cast<Sentence const&>(source))
,m_initialTargetPhrase(Output)
{
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.SetMaxHypoStackSize(m_staticData.GetMaxHypoStackSize());
		sourceHypoColl.SetBeamThreshold(m_staticData.GetBeamThreshold());
	}
}

Manager::~Manager() {}

/**
 * this is the main loop when a sentence is translated
 * hypotheses are expanded stack by stack, until the end of the sentence
 */
void Manager::ProcessSentence()
{	
	m_staticData.GetSentenceStats().ZeroAll();
	list < DecodeStep > &decodeStepList = m_staticData.GetDecodeStepList();
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_possibleTranslations.CreateTranslationOptions(decodeStepList
  														, m_staticData.GetAllLM()
  														, m_staticData.GetFactorCollection()
  														, m_staticData.GetWeightWordPenalty()
  														, m_staticData.GetDropUnknown()
  														, m_staticData.GetVerboseLevel());

	// initial seed hypothesis: nothing translated, no words produced
	{
		Hypothesis *hypo = Hypothesis::Create(m_source, m_initialTargetPhrase);
		m_hypoStack[0].AddPrune(hypo);
	}
	
	// go through each stack
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
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
				ProcessOneHypothesis(hypothesis); // expand the hypothesis
			}
		// some logging
		if (m_staticData.GetVerboseLevel() > 0) {
			//OutputHypoStack();
			OutputHypoStackSize();
		}

	}

	// some more logging
	if (m_staticData.GetVerboseLevel() > 0) {
	  cerr << m_staticData.GetSentenceStats();
    cerr << "Hypotheses created since startup: "<< Hypothesis::s_HypothesesCreated<<endl;
		//OutputHypoStack();
		//OutputHypoStackSize();
	}
}

/** 
 * find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits.
 */
void Manager::ProcessOneHypothesis(const Hypothesis &hypothesis)
{
	// since we check for reordering limits, its good to have that limit handy
	int maxDistortion = m_staticData.GetMaxDistortion();

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
												, m_possibleTranslations.GetTranslationOptionList(WordsRange(startPos, endPos)));
				}
			}
		}

		return; // done with special case (no reordering limit)
	}

	// if there are reordering limits, make sure it is not violated
	// the coverage bitmap is handy here (and the position of the first gap)
	const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
	const size_t hypoWordCount		= hypoBitmap.GetWordsCount()
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
												,m_possibleTranslations.GetTranslationOptionList(WordsRange(startPos, endPos)));
				}
			}
			// filling in gap => just check for overlap
			else if (startPos < hypoWordCount)
				{
					if (startPos >= hypoFirstGapPos
						&& !hypoBitmap.Overlap(WordsRange(startPos, endPos)))
					{
						ExpandAllHypotheses(hypothesis
													,m_possibleTranslations.GetTranslationOptionList(WordsRange(startPos, endPos)));
					}
				}
			// ignoring, continuing forward => be limited by start of gap
			else
				{
					if (endPos <= hypoFirstGapPos + maxDistortion
						&& !hypoBitmap.Overlap(WordsRange(startPos, endPos)))
					{
						ExpandAllHypotheses(hypothesis
													,m_possibleTranslations.GetTranslationOptionList(WordsRange(startPos, endPos)));
					}
				}
		}
	}
}

void Manager::ExpandAllHypotheses(const Hypothesis &hypothesis,const TranslationOptionList &transOptList)
{
	TranslationOptionList::const_iterator iter;
	for (iter = transOptList.begin() ; iter != transOptList.end() ; ++iter)
	{
		ExpandHypothesis(hypothesis, **iter);
	}
}

/**
 * Expand one hypothesis with a translation option.
 * this involves initial creation, scoring and adding it to the proper stack
 */
void Manager::ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt) 
{
	// create hypothesis and calculate all its scores
	Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
	newHypo->CalcScore(m_staticData, m_possibleTranslations.GetFutureScore());
	
	// logging for the curious
	if(m_staticData.GetVerboseLevel() > 2) 
		{			
			newHypo->PrintHypothesis(m_source, m_staticData.GetWeightDistortion(), m_staticData.GetWeightWordPenalty());
		}

	// add to hypothesis stack
	size_t wordsTranslated = newHypo->GetWordsBitmap().GetWordsCount();	
	m_hypoStack[wordsTranslated].AddPrune(newHypo);
}

/**
 * Find best hypothesis on the last steck
 * this is the end point of the best translation
 */
const Hypothesis *Manager::GetBestHypothesis() const
{
	const HypothesisCollection &hypoColl = m_hypoStack.back();
	return hypoColl.GetBestHypothesis();
}

/**
 * Logging of hypotheses stacks and their sizes
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
 * the heavy lifting is done in the LatticePath and LatticePathCollection
 * this function controls this for one sentence
 */
void Manager::CalcNBest(size_t count, LatticePathList &ret) const
{
#ifdef N_BEST
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
#endif
}
