// $Id: Manager.cpp 215 2007-11-20 17:25:55Z hieu $
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
#include "TrellisPath.h"
#include "TrellisPathCollection.h"
#include "TranslationOption.h"
#include "LMList.h"
#include "TranslationOptionCollection.h"
#include "StaticData.h"
#include "DecodeStepTranslation.h"

using namespace std;

Manager::Manager(InputType const& source)
:m_source(source)
,m_hypoStackColl(source.GetSize()
								, StaticData::Instance().GetDecodeStepCollection()
								, StaticData::Instance().GetMaxHypoStackSize()
								, StaticData::Instance().GetBeamThreshold())
,m_transOptColl(source.CreateTranslationOptionCollection())
,m_initialTargetPhrase(Output)
{
	TRACE_ERR("Start decoding: " << source << endl);
	const StaticData &staticData = StaticData::Instance();
	staticData.InitializeBeforeSentenceProcessing(source);
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
	const StaticData &staticData = StaticData::Instance();
	staticData.ResetSentenceStats(m_source);
	const DecodeStepCollection &decodeStepList = staticData.GetDecodeStepCollection();
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_transOptColl->CreateTranslationOptions(decodeStepList);
	//TRACE_ERR(*m_transOptColl << endl);

	// initial seed hypothesis: nothing translated, no words produced
	Hypothesis *seedHypo = Hypothesis::Create(m_source, decodeStepList, m_initialTargetPhrase);
	m_hypoStackColl.GetStack(0).AddPrune(seedHypo);
	
	size_t colIndex		= 0
				, rowIndex 	= 0;

	// go through each stack	
	for (size_t stackNo = 0 ; stackNo < m_hypoStackColl.GetSize() - 1 ; ++stackNo)
	{
		HypothesisStack &hypoStack = m_hypoStackColl.GetStack(stackNo);

		TRACE_ERR("processing hypothesis from stack " << stackNo 
							<< "(" << rowIndex << "," << colIndex << ") of " << m_source.GetSize() << endl);
		// the stack is pruned before processing (lazy pruning):
		hypoStack.PreProcess();
		
		// go through each hypothesis on the stack and try to expand it
		HypothesisStack::const_iterator iterHypo;
		for (iterHypo = hypoStack.begin() ; iterHypo != hypoStack.end() ; ++iterHypo)
		{
			Hypothesis &hypothesis = **iterHypo;
			//TRACE_ERR(hypothesis << endl);			
			ProcessOneHypothesis(hypothesis, decodeStepList, stackNo); // expand the hypothesis
		}

		if (stackNo % STACK_CLEAROUT == (STACK_CLEAROUT - 1))
		{
			RemoveDeadendHypotheses(stackNo);
		}

		// some logging
		m_hypoStackColl.OutputHypoStackSize(true, m_source.GetSize());
		//TRACE_ERR(m_hypoStackColl);
		//m_hypoStackColl.OutputArcListSize();
		
 		if (++colIndex > m_source.GetSize())
		{
			colIndex = 0;
			++rowIndex;
		}
	}

	if (staticData.GetNBestSize() > 0) // don't touch the last stack
		RemoveDeadendHypotheses(m_hypoStackColl.GetSize() - 2);
	else // remove everything except the best hypo, & its antecedents
		RemoveDeadendHypotheses(m_hypoStackColl.GetSize() - 1, GetBestHypothesis());

	// last stack
	HypothesisStack &lastHypoColl = m_hypoStackColl.GetStack(m_hypoStackColl.GetSize() - 1);
	lastHypoColl.PreProcess();
	
	//OutputHypoStack();
	m_hypoStackColl.OutputHypoStackSize(true, m_source.GetSize());
	//OutputArcListSize();
	
	// some more logging
	VERBOSE(2, StaticData::Instance().GetSentenceStats());
}

// helper function
int DistanceFromDiagonal(size_t stackNo, size_t sourceSize)
{
	size_t noDecodeStep = StaticData::Instance().GetDecodeStepCollection().GetSize();
	
	if (noDecodeStep == 2)
	{ // shortcut
		int yCoord = (int) (stackNo / (sourceSize + 1));
		int xCoord = (int) (stackNo - (yCoord * (sourceSize + 1)));
	  return xCoord - yCoord;
	}
	
	if (noDecodeStep == 1)
	{
		return (int) stackNo;
	}

	// general case. prob wrong...
	assert(noDecodeStep == 2);
  
	/*
	size_t cubeLength   = sourceSize + noDecodeStep - 1;
	vector<int> cubeIndex(noDecodeStep);

	int remainder = (int) stackNo;
	for (int i = 0 ; i < (int) noDecodeStep ; ++i)
	{
		int divisor = (int) (cubeLength ^ (noDecodeStep - 1 - i));
		cubeIndex[i] = remainder / divisor;
		remainder = remainder % divisor;
	}
	return (decodeStepId==0) ? cubeIndex[1] - cubeIndex[0]
													:  cubeIndex[0] - cubeIndex[1];
	*/
	return 0;
}

/** Find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits. 
 * \param hypothesis hypothesis to be expanded upon
 */
void Manager::ProcessOneHypothesis(const Hypothesis &hypothesis
																	, const DecodeStepCollection &decodeStepList
																	, size_t stackNo)
{
	DecodeStepCollection::const_iterator iter;
	for (iter = decodeStepList.begin() ; iter != decodeStepList.end() ; ++iter)
	{
		const DecodeStepTranslation &decodeStep	= **iter;
		size_t decodeStepId						= decodeStep.GetId();
		int diagSlack = (int) StaticData::Instance().GetDiagSlack();
		int distFromDiagonal = DistanceFromDiagonal(stackNo, m_source.GetSize());

		switch (StaticData::Instance().GetAsyncMethod())
		{
			case Multipass:
			case MultipassLarge1st:
			case MultipassLargeLast:
				// finish 1st decode step before doing next
				if (decodeStepId > 0 && stackNo < m_source.GetSize())
					continue;
				break;
			case UpperDiagonal:
      {
				if (distFromDiagonal > diagSlack && decodeStepId == 0)
				{
					continue;
				}
				break;
      }
      case AroundDiagonal:
      {
				abort();
				/*
        int diagSlack = (int) StaticData::Instance().GetAsyncMethodVar();
        int distFromDiagonal = DistanceFromDiagonal(stackNo, m_source.GetSize(), decodeStepId);
				if (distFromDiagonal > diagSlack)
        {
          continue;
        }
				*/
        break;
      }
			case NonTiling:
			{
				if ( (distFromDiagonal > 0 && decodeStepId == 0)
						||(distFromDiagonal == 0 && decodeStepId > 0) )
				{ 
					continue;
				}
				break;
			}
			case MultipleFirstStep:
			{
				if (distFromDiagonal > diagSlack && decodeStepId == 0)
				{
					continue;
				}
				break;
			}
			default:
				abort();
		}
		// since we check for reordering limits, its good to have that limit handy
		int maxDistortion = StaticData::Instance().GetMaxDistortion(decodeStepId);

		// if there are reordering limits, make sure it is not violated
		// the coverage bitmap is handy here (and the position of the first gap)
		const WordsBitmap &hypoBitmap = hypothesis.GetSourceBitmap();

		const size_t hypoWordCount		= hypoBitmap.GetNumWordsCovered(decodeStepId)
								, hypoFirstGapPos	= hypoBitmap.GetFirstGapPos(decodeStepId)
								, sourceSize			= m_source.GetSize();
		
		// MAIN LOOP. go through each possible hypo
		for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < sourceSize ; ++endPos)
			{				
				WordsRange transOptRange(decodeStepId, startPos, endPos);
				if (hypoBitmap.Overlap(transOptRange))
					break;
				if (!hypoBitmap.IsHierarchy(decodeStepId, startPos, endPos))
					break;
				//if (!hypothesis.OverlapPreviousStep(decodeStepId, startPos, endPos))
				//	continue;
				
				bool expandHypo = false;

				
				if (maxDistortion < 0)
				{ // no re-ordering
					expandHypo = true;
				}
				else if (hypoFirstGapPos == hypoWordCount)
				{ // no gap so far => don't skip more than allowed limit
					if (startPos == hypoWordCount
							|| (startPos > hypoWordCount 
									&& endPos <= hypoWordCount + maxDistortion)
						)
					{
						expandHypo = true;
					}
				}
				// filling in gap => just check for overlap
				else if (startPos < hypoWordCount)
				{
					if (startPos >= hypoFirstGapPos)
					{
						expandHypo = true;
					}
				}
				// ignoring, continuing forward => be limited by start of gap
				else
				{
					if (endPos <= hypoFirstGapPos + maxDistortion)
					{
						expandHypo = true;
					}
				}

				if (expandHypo)
					ExpandAllHypotheses(hypothesis
														, m_transOptColl->GetTranslationOptionList(transOptRange)
														, distFromDiagonal);

			}
		}
	}
}

/**
 * Expand a hypothesis given a list of translation options
 * \param hypothesis hypothesis to be expanded upon
 * \param transOptList list of translation options to be applied
 */

void Manager::ExpandAllHypotheses(const Hypothesis &hypothesis
																	, const TranslationOptionList &transOptList
																	, size_t distFromDiagonal)
{
	
	bool go = false;
	if (go)
		TRACE_ERR("  hypo = " << hypothesis << endl);
	
	//TRACE_ERR("transOptList = " << transOptList.GetSize() << endl);
	
	TranslationOptionList::const_iterator iter;
	for (iter = transOptList.begin() ; iter != transOptList.end() ; ++iter)
	{
		if (go)	
			TRACE_ERR("  trans = " << **iter << endl);
		ExpandHypothesis(hypothesis, **iter, distFromDiagonal);
	}
}


/**
 * Expand one hypothesis with a translation option.
 * this involves initial creation, scoring and adding it to the proper stack
 * \param hypothesis hypothesis to be expanded upon
 * \param transOpt translation option (phrase translation) 
 *        that is applied to create the new hypothesis
 */

void Manager::ExpandHypothesis(const Hypothesis &hypothesis
															, const TranslationOption &transOpt
															, size_t distFromDiagonal) 
{
	const AlignmentPair
							&hypoAlignment		= hypothesis.GetAlignmentPair()
						, &transOptAlignment= transOpt.GetAlignmentPair();
	const Phrase &transOptPhrase	= transOpt.GetTargetPhrase();
	size_t 	decodeStepId					= transOpt.GetDecodeStepId()
					, sourceStart 				= transOpt.GetStartPos()
					, targetStart					= hypothesis.GetNextStartPos(transOpt);
						
	if (hypoAlignment.IsCompatible(transOptAlignment
																	, sourceStart
																	, targetStart)
			&& hypothesis.GetTargetPhrase().IsCompatible(transOptPhrase, targetStart)
			&& (StaticData::Instance().GetAsyncMethod() != MultipleFirstStep 
											|| decodeStepId == 0
											|| transOpt.GetSourceWordsRange().GetNumWordsCovered() == distFromDiagonal))
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
			newHypo->PrintHypothesis();
		}

		// add to hypothesis stack
		m_hypoStackColl.AddPrune(newHypo);
	}
}

/**
 * Find best hypothesis on the last stack.
 * This is the end point of the best translation, which can be traced back from here
 */
const Hypothesis *Manager::GetBestHypothesis() const
{
	const HypothesisStack &hypoColl = m_hypoStackColl.GetOutputStack();
	return hypoColl.GetBestHypothesis();
}

/**
 * After decoding, the hypotheses in the stacks and additional arcs
 * form a search graph that can be mined for n-best lists.
 * The heavy lifting is done in the TrellisPath and TrellisPathCollection
 * this function controls this for one sentence.
 *
 * \param count the number of n-best translations to produce
 * \param ret holds the n-best list that was calculated
 */
void Manager::CalcNBest(size_t count, TrellisPathList &ret,bool onlyDistinct) const
{
	if (count <= 0)
		return;

	vector<const Hypothesis*> sortedPureHypo = m_hypoStackColl.GetOutputStack().GetSortedList();

	if (sortedPureHypo.size() == 0)
		return;


	TrellisPathCollection contenders;	
	set<Phrase> distinctHyps;

	// add all pure paths
	vector<const Hypothesis*>::const_iterator iterBestHypo;
	for (iterBestHypo = sortedPureHypo.begin() 
			; iterBestHypo != sortedPureHypo.end()
			; ++iterBestHypo)
	{
		contenders.Add(new TrellisPath(*iterBestHypo));
	}
	TRACE_ERR("Num of n-best contenders: " << contenders.GetSize() << " ");

	// MAIN loop
	for (size_t iteration = 0 ; (onlyDistinct ? distinctHyps.size() : ret.GetSize()) < count && contenders.GetSize() > 0 && (iteration < count * 20) ; iteration++)
	{
		// get next best from list of contenders
		TrellisPath *path = contenders.pop();
		assert(path);
		if(onlyDistinct)
		{
			Phrase tgtPhrase = path->GetSurfacePhrase();
			distinctHyps.insert(tgtPhrase).second;
		}
		
		ret.Add(path);
		// create deviations from current best
		path->CreateDeviantPaths(contenders);		
				
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

void Manager::RemoveDeadendHypotheses(size_t stackNo, const Hypothesis *excludeHypo)
{
	AsyncMethod asyncMethod = StaticData::Instance().GetAsyncMethod();
	
	for (int currStackNo = (int) stackNo ; currStackNo > 0 ; --currStackNo)
	{
		if (asyncMethod != MultipassLargeLast && currStackNo != m_source.GetSize())
		{
			HypothesisStack &hypoStack = m_hypoStackColl.GetStack(currStackNo);
			hypoStack.RemoveDeadendHypotheses(excludeHypo);
		}
	}
}

