// $Id$
// vim:tabstop=2

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
#include "StaticData.h"

using namespace std;

Manager::Manager(InputType const& source)
:m_source(source)
,m_hypoStack(source.GetSize(), StaticData::Instance().GetDecodeStepList())
,m_transOptColl(source.CreateTranslationOptionCollection())
,m_initialTargetPhrase(Output)
{
	TRACE_ERR("Start decoding: " << source << endl);
	const StaticData &staticData = StaticData::Instance();
	staticData.InitializeBeforeSentenceProcessing(source);

	HypothesisStack::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.SetMaxHypoStackSize(staticData.GetMaxHypoStackSize());
		sourceHypoColl.SetBeamThreshold(staticData.GetBeamThreshold());
	}
}

Manager::~Manager() 
{
  delete m_transOptColl;
	StaticData::Instance().CleanUpAfterSentenceProcessing();      
  TRACE_ERR("Completed decoding" << endl);
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void Manager::ProcessSentence()
{	
	StaticData::Instance().ResetSentenceStats(m_source);
	const vector<DecodeStep*> &decodeStepList = StaticData::Instance().GetDecodeStepList();
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_transOptColl->CreateTranslationOptions(decodeStepList);

	// initial seed hypothesis: nothing translated, no words produced
	Hypothesis *seedHypo = Hypothesis::Create(m_source, decodeStepList, m_initialTargetPhrase);
	m_hypoStack.GetStack(0).AddPrune(seedHypo);
	
	// go through each stack	
	for (size_t stackNo = 0 ; stackNo < m_hypoStack.GetSize() - 1 ; ++stackNo)
	{
		HypothesisCollection &sourceHypoColl = m_hypoStack.GetStack(stackNo);

		// the stack is pruned before processing (lazy pruning):
		VERBOSE(3,"processing hypothesis from next stack");
		sourceHypoColl.PruneToSize(StaticData::Instance().GetMaxHypoStackSize());
		VERBOSE(3,std::endl);
		sourceHypoColl.CleanupArcList();

		// go through each hypothesis on the stack and try to expand it
		HypothesisCollection::const_iterator iterHypo;
		for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
		{
			Hypothesis &hypothesis = **iterHypo;
			ProcessOneHypothesis(hypothesis, decodeStepList); // expand the hypothesis
		}

		RemoveDeadendHypotheses(stackNo);
		
		// some logging
		IFVERBOSE(2) { OutputHypoStackSize(); }
		//OutputHypoStackSize();
		//OutputArcListSize();
	}

	// last stack
	HypothesisCollection &lastHypoColl = m_hypoStack.GetStack(m_hypoStack.GetSize() - 1);
	lastHypoColl.PruneToSize(StaticData::Instance().GetMaxHypoStackSize());
	lastHypoColl.CleanupArcList();

	//OutputHypoStack();
	OutputHypoStackSize();
	//OutputArcListSize();
	
	// some more logging
	VERBOSE(2, StaticData::Instance().GetSentenceStats());
}

/** Find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits. 
 * \param hypothesis hypothesis to be expanded upon
 */
void Manager::ProcessOneHypothesis(const Hypothesis &hypothesis, const std::vector<DecodeStep*> &decodeStepList)
{	
	// since we check for reordering limits, its good to have that limit handy
	int maxDistortion = StaticData::Instance().GetMaxDistortion();

	// no limit of reordering: only check for overlap
	if (maxDistortion < 0)
	{	
		const WordsBitmap &hypoBitmap	= hypothesis.GetSourceBitmap();

		std::vector<DecodeStep*>::const_iterator iter;
		for (iter = decodeStepList.begin() ; iter != decodeStepList.end() ; ++iter)
		{
			const DecodeStep &decodeStep	= **iter;
			size_t decodeStepId						= decodeStep.GetId();
			const size_t hypoFirstGapPos	= hypoBitmap.GetFirstGapPos(decodeStepId)
									, sourceSize			= m_source.GetSize();

			for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
			{
				for (size_t endPos = startPos ; endPos < sourceSize ; ++endPos)
				{
					if (!hypoBitmap.IsHierarchy(decodeStepId, startPos, endPos))
						break;

					if (!hypoBitmap.Overlap(WordsRange(decodeStepId, startPos, endPos)))
					{
						ExpandAllHypotheses(hypothesis
													, m_transOptColl->GetTranslationOptionList(
																																WordsRange(decodeStepId, startPos, endPos)));
					}
				}

			}
		}
		return; // done with special case (no reordering limit)
	}

	// if there are reordering limits, make sure it is not violated
	// the coverage bitmap is handy here (and the position of the first gap)
	const WordsBitmap &hypoBitmap = hypothesis.GetSourceBitmap();

	std::vector<DecodeStep*>::const_iterator iter;
	for (iter = decodeStepList.begin() ; iter != decodeStepList.end() ; ++iter)
	{
		const DecodeStep &decodeStep	= **iter;
		size_t decodeStepId						= decodeStep.GetId();
		const size_t hypoWordCount		= hypoBitmap.GetNumWordsCovered(decodeStepId)
								, hypoFirstGapPos	= hypoBitmap.GetFirstGapPos(decodeStepId)
								, sourceSize			= m_source.GetSize();
		
		// MAIN LOOP. go through each possible hypo
		for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < sourceSize ; ++endPos)
			{					
				if (!hypoBitmap.IsHierarchy(decodeStepId, startPos, endPos))
					break;

				// no gap so far => don't skip more than allowed limit
				if (hypoFirstGapPos == hypoWordCount)
				{
					if (startPos == hypoWordCount
							|| (startPos > hypoWordCount 
									&& endPos <= hypoWordCount + maxDistortion)
						)
					{
						ExpandAllHypotheses(hypothesis
													,m_transOptColl->GetTranslationOptionList(WordsRange(decodeStepId, startPos, endPos)));
					}
				}
				// filling in gap => just check for overlap
				else if (startPos < hypoWordCount)
				{
					if (startPos >= hypoFirstGapPos
						&& !hypoBitmap.Overlap(WordsRange(decodeStepId, startPos, endPos)))
					{
						ExpandAllHypotheses(hypothesis
													,m_transOptColl->GetTranslationOptionList(WordsRange(decodeStep.GetId(), startPos, endPos)));
					}
				}
				// ignoring, continuing forward => be limited by start of gap
				else
				{
					if (endPos <= hypoFirstGapPos + maxDistortion
							&& !hypoBitmap.Overlap(WordsRange(decodeStepId, startPos, endPos)))
					{
						ExpandAllHypotheses(hypothesis
														,m_transOptColl->GetTranslationOptionList(WordsRange(decodeStepId, startPos, endPos)));
					}
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
 * \param hypothesis hypothesis to be expanded upon
 * \param transOpt translation option (phrase translation) 
 *        that is applied to create the new hypothesis
 */

void Manager::ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt) 
{
	const AlignmentPair
							&hypoAlignment		= hypothesis.GetAlignmentPair()
						, &targetAlignment	= transOpt.GetAlignmentPair();
	size_t 	decodeStepId					= transOpt.GetDecodeStepId()
					, sourceStart 				= transOpt.GetStartPos()
					, targetStart					= hypothesis.GetNextStartPos(transOpt);
					
	if (decodeStepId == INITIAL_DECODE_STEP_ID ||  hypoAlignment.IsCompatible(
																	targetAlignment
																	, sourceStart
																	, targetStart))
	{
		// create hypothesis and calculate all its scores
		Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
	
		// alignments don't allow hypo to be decoded to completion
		if (!newHypo->IsCompletable())
		{
			FREEHYPO(newHypo);
			return;
		}

		newHypo->CalcScore(m_transOptColl->GetFutureScoreObject());
		// logging for the curious
		IFVERBOSE(3) {
			newHypo->PrintHypothesis(m_source, StaticData::Instance().GetWeightDistortion(), StaticData::Instance().GetWeightWordPenalty());
		}

		// add to hypothesis stack
		m_hypoStack.AddPrune(newHypo);
	}
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
void Manager::OutputHypoStackSize() const
{
	HypothesisStack::const_iterator iterStack;
	TRACE_ERR( "Stack sizes: " << endl);
	int sqSize	= (int) m_source.GetSize() + 1
			, i 		= 1;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		TRACE_ERR((*iterStack).size() << "\t");
		if (i++ % sqSize == 0)
			TRACE_ERR( endl);
	}
}

/**
 * Logging of hypothesis stack contents
 * \param stack number of stack to be reported, report all stacks if 0 
 */
void Manager::OutputHypoStack(int stack)
{
	if (stack >= 0)
	{
		TRACE_ERR( "Stack " << stack << ": " << endl << m_hypoStack.GetStack(stack) << endl);
	}
	else
	{ // all stacks
		int i = 0;
		HypothesisStack::iterator iterStack;
		for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
		{
			HypothesisCollection &hypoColl = *iterStack;
			TRACE_ERR( "Stack " << i++ << ": " << endl << hypoColl << endl);
		}
	}
}

/** Output arc list information to debug mem leak */
void Manager::OutputArcListSize() const
{
	TRACE_ERR( "Arc sizes: ");
	
	HypothesisStack::const_iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		const HypothesisCollection &hypoColl = *iterStack;
		
		size_t arcCount = 0;
		HypothesisCollection::const_iterator iterColl;
		for (iterColl = hypoColl.begin() ; iterColl != hypoColl.end() ; ++iterColl)
		{
			Hypothesis *hypo = *iterColl;
			const ArcList *arcList = hypo->GetArcList();
			if (arcList != NULL)
				arcCount += arcList->size();
		}
		
		TRACE_ERR( ", " << arcCount);
	}
	TRACE_ERR(endl);
}

void GetSurfacePhrase(std::vector<size_t>& tphrase, LatticePath const& path)
{
	tphrase.clear();
	const Phrase &targetPhrase = path.GetTargetPhrase();

	for (size_t pos = 0 ; pos < targetPhrase.GetSize() ; ++pos)
	{
		const Factor *factor = targetPhrase.GetFactor(pos, 0);
		assert(factor);
		tphrase.push_back(factor->GetId());
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
void Manager::CalcNBest(size_t count, LatticePathList &ret,bool onlyDistinct) const
{
	if (count <= 0)
		return;

	vector<const Hypothesis*> sortedPureHypo = m_hypoStack.back().GetSortedList();

	if (sortedPureHypo.size() == 0)
		return;


	LatticePathCollection contenders;	
	set<std::vector<size_t> > distinctHyps;

	// add all pure paths
	vector<const Hypothesis*>::const_iterator iterBestHypo;
	for (iterBestHypo = sortedPureHypo.begin() 
			; iterBestHypo != sortedPureHypo.end()
			; ++iterBestHypo)
	{
		contenders.Add(new LatticePath(*iterBestHypo));
	}
	TRACE_ERR("Num of n-best contenders: " << contenders.GetSize() << " ");

	// MAIN loop
	for (size_t iteration = 0 ; (onlyDistinct ? distinctHyps.size() : ret.GetSize()) < count && contenders.GetSize() > 0 && (iteration < count * 20) ; iteration++)
	{
		// get next best from list of contenders
		LatticePath *path = contenders.pop();
		assert(path);
		bool addPath = true;
		if(onlyDistinct)
		{
			// TODO - not entirely correct.
			// output phrase can't be assumed to only contain factor 0.
			// have to look in StaticData.GetOutputFactorOrder() to find out what output factors should be
			std::vector<size_t> tgtPhrase;
			GetSurfacePhrase(tgtPhrase,*path);
			addPath = distinctHyps.insert(tgtPhrase).second;
		}
		
		if(addPath && ret.Add(path)) 
		{	// create deviations from current best
			path->CreateDeviantPaths(contenders);		
		}
		else
			delete path;
		
		TRACE_ERR(contenders.GetSize() << " ");
		
		if(onlyDistinct)
		{
			size_t nBestFactor = StaticData::Instance().GetNBestFactor();
			if (nBestFactor > 0)
				contenders.Prune(count * nBestFactor);
		}
		else
		{
			contenders.Prune(count);
		}
	}
	
	TRACE_ERR(contenders.GetSize() << endl);
}

void Manager::CalcDecoderStatistics() const 
{
  const Hypothesis *hypo = GetBestHypothesis();
	if (hypo != NULL && hypo->GetSize() > 0)
  {
		StaticData::Instance().GetSentenceStats().CalcFinalStats(*hypo);
    IFVERBOSE(2) {
		 	if (hypo != NULL) {
		   	string buff;
		  	string buff2;
		   	TRACE_ERR( "Source and Target Units:"
		 							<< hypo->GetSourcePhrase());
				buff2.insert(0,"] ");
				buff2.insert(0,(hypo->GetCurrTargetPhrase()).ToString());
				buff2.insert(0,":");
				buff2.insert(0,(hypo->GetCurrSourceWordsRange()).ToString());
				buff2.insert(0,"[");
				
				hypo = hypo->GetPrevHypo();
				while (hypo != NULL) {
					//dont print out the empty final hypo
				  buff.insert(0,buff2);
				  buff2.clear();
				  buff2.insert(0,"] ");
				  buff2.insert(0,(hypo->GetCurrTargetPhrase()).ToString());
				  buff2.insert(0,":");
				  buff2.insert(0,(hypo->GetCurrSourceWordsRange()).ToString());
				  buff2.insert(0,"[");
				  hypo = hypo->GetPrevHypo();
				}
				TRACE_ERR( buff << endl);
      }
    }
  }
}

void Manager::RemoveDeadendHypotheses(size_t stackNo)
{
	for (int currStackNo = (int) stackNo ; currStackNo > 0 ; --currStackNo)
	{
		HypothesisCollection &hypoColl = m_hypoStack.GetStack(currStackNo);
		
		HypothesisCollection::iterator iter = hypoColl.begin();
		while (iter != hypoColl.end())
		{
			const Hypothesis *hypo = *iter;
			if (hypo->GetRefCount() == 0)
			{
				HypothesisCollection::iterator iterDelete = iter++;
				hypoColl.Remove(iterDelete);
			}
			else
			{
				++iter;
			}
		}
	}
}

