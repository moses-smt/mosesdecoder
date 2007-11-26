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
#ifdef WIN32
#include <hash_set>
#else
#include <ext/hash_set>
#endif

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

using namespace std;

#undef DEBUGLATTICE
#ifdef DEBUGLATTICE
static bool debug2 = false;
#endif

Manager::Manager(InputType const& source)
:m_source(source)
,m_hypoStackColl(source.GetSize() + 1)
,m_transOptColl(source.CreateTranslationOptionCollection())
,m_initialTargetPhrase(Output)
,m_start(clock())
{
	VERBOSE(1, "Translating: " << m_source << endl);
	const StaticData &staticData = StaticData::Instance();
	staticData.InitializeBeforeSentenceProcessing(source);

	std::vector < HypothesisStack >::iterator iterStack;
	for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{
		HypothesisStack &sourceHypoColl = *iterStack;
		sourceHypoColl.SetMaxHypoStackSize(staticData.GetMaxHypoStackSize());
		sourceHypoColl.SetBeamWidth(staticData.GetBeamWidth());
	}
}

Manager::~Manager() 
{
  delete m_transOptColl;
	StaticData::Instance().CleanUpAfterSentenceProcessing();      

	clock_t end = clock();
	float et = (end - m_start);
	et /= (float)CLOCKS_PER_SEC;
	VERBOSE(1, "Translation took " << et << " seconds" << endl);
	VERBOSE(1, "Finished translating" << endl);
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void Manager::ProcessSentence()
{	
	const StaticData &staticData = StaticData::Instance();
	staticData.ResetSentenceStats(m_source);
	const vector <DecodeGraph*>
			&decodeStepVL = staticData.GetDecodeStepVL();
	
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_transOptColl->CreateTranslationOptions(decodeStepVL);

	// initial seed hypothesis: nothing translated, no words produced
	{
		Hypothesis *hypo = Hypothesis::Create(m_source, m_initialTargetPhrase);
		m_hypoStackColl[0].AddPrune(hypo);
	}
	
	// go through each stack
	std::vector < HypothesisStack >::iterator iterStack;
	for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{
		HypothesisStack &sourceHypoColl = *iterStack;

		// the stack is pruned before processing (lazy pruning):
		VERBOSE(3,"processing hypothesis from next stack");
		sourceHypoColl.PruneToSize(staticData.GetMaxHypoStackSize());
		VERBOSE(3,std::endl);
		sourceHypoColl.CleanupArcList();
		// go through each hypothesis on the stack and try to expand it
		HypothesisStack::const_iterator iterHypo;
		for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
			{
				Hypothesis &hypothesis = **iterHypo;
				ProcessOneHypothesis(hypothesis); // expand the hypothesis
			}
		// some logging
		IFVERBOSE(2) { OutputHypoStackSize(); }
	}

	// some more logging
	VERBOSE(2, staticData.GetSentenceStats());
}

/** Find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits. 
 * \param hypothesis hypothesis to be expanded upon
 */
void Manager::ProcessOneHypothesis(const Hypothesis &hypothesis)
{
	// since we check for reordering limits, its good to have that limit handy
	int maxDistortion = StaticData::Instance().GetMaxDistortion();
	bool isWordLattice = StaticData::Instance().GetInputType() == WordLatticeInput;

	// no limit of reordering: only check for overlap
	if (maxDistortion < 0)
	{	
		const WordsBitmap hypoBitmap	= hypothesis.GetWordsBitmap();
		const size_t hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
								, sourceSize			= m_source.GetSize();

		for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
		{
      size_t maxSize = sourceSize - startPos;
      size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;

			for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
			{
				if (!hypoBitmap.Overlap(WordsRange(startPos, endPos)))
				{
					ExpandAllHypotheses(hypothesis
												, m_transOptColl->GetTranslationOptionList(WordsRange(startPos, endPos)));
				}
			}
		}

		return; // done with special case (no reordering limit)
	}

	// if there are reordering limits, make sure it is not violated
	// the coverage bitmap is handy here (and the position of the first gap)
	const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
	const size_t	hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
							, sourceSize			= m_source.GetSize();
	
	// MAIN LOOP. go through each possible hypo
	for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
	{
    size_t maxSize = sourceSize - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
#ifdef DEBUGLATTICE
		const int INTEREST = 114;
#endif
    maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;
		if (isWordLattice) {
			// first question: is there a path from the closest translated word to the left
			// of the hypothesized extension to the start of the hypothesized extension?
			size_t closestLeft = hypoBitmap.GetEdgeToTheLeftOf(startPos);
			if (closestLeft != startPos && closestLeft != 0 && !m_source.CanIGetFromAToB(closestLeft+1, startPos+1)) {
#ifdef DEBUGLATTICE
			  if (startPos == INTEREST) {
				  std::cerr << hypothesis <<"\n";
				  std::cerr << "Die0: " << (closestLeft) << " " << startPos << "\n";
				}
#endif
			  continue;
			}
		}

		for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
		{
			// check for overlap
		  WordsRange extRange(startPos, endPos);
#ifdef DEBUGLATTICE
	    bool debug = (startPos > (INTEREST-25) && hypoFirstGapPos > 0 && startPos <= INTEREST && endPos >=INTEREST && endPos < (INTEREST+150) && hypoFirstGapPos == INTEREST);
	    debug2 = debug && (startPos==INTEREST && endPos >=INTEREST);
			if (debug) { std::cerr << (startPos==INTEREST? "LOOK-->" : "") << "XP: " << hypothesis << "\next: " << extRange << "\n"; }
#endif
			if (hypoBitmap.Overlap(extRange) ||
			      (isWordLattice && (!m_source.IsCoveragePossible(extRange) ||
					                     !m_source.IsExtensionPossible(hypothesis.GetCurrSourceWordsRange(), extRange))
					  )
			   )
		  {
#ifdef DEBUGLATTICE
			  if (debug) { std::cerr << "Die1\n"; }
#endif
			  continue;
			}
		  // TODO ask second question here
			if (isWordLattice) {
				size_t closestRight = hypoBitmap.GetEdgeToTheRightOf(endPos);
				if (closestRight != endPos && closestRight != sourceSize && !m_source.CanIGetFromAToB(endPos, closestRight)) {
#ifdef DEBUGLATTICE
			    if (debug) { std::cerr << "Can't get to right edge\n"; }
#endif
				  continue;
				}
			}
			
			bool leftMostEdge = (hypoFirstGapPos == startPos);
			
			// any length extension is okay if starting at left-most edge
			if (leftMostEdge)
			{
#ifdef DEBUGLATTICE
			  size_t vl = StaticData::Instance().GetVerboseLevel();
			  if (debug2) { std::cerr << "Ext!\n"; StaticData::Instance().SetVerboseLevel(4); }
#endif
				ExpandAllHypotheses(hypothesis
							,m_transOptColl->GetTranslationOptionList(extRange));
#ifdef DEBUGLATTICE
			  StaticData::Instance().SetVerboseLevel(vl);
#endif
			}
			// starting somewhere other than left-most edge, use caution
			else
			{
				// the basic idea is this: we would like to translate a phrase starting
				// from a position further right than the left-most open gap. The
				// distortion penalty for the following phrase will be computed relative
				// to the ending position of the current extension, so we ask now what
				// its maximum value will be (which will always be the value of the
				// hypothesis starting at the left-most edge).  If this vlaue is than
				// the distortion limit, we don't allow this extension to be made.
				WordsRange bestNextExtension(hypoFirstGapPos, hypoFirstGapPos);
				int required_distortion =
					m_source.ComputeDistortionDistance(extRange, bestNextExtension);

				if (required_distortion <= maxDistortion) {
					ExpandAllHypotheses(hypothesis
								,m_transOptColl->GetTranslationOptionList(extRange));
				}
#ifdef DEBUGLATTICE
				else
			    if (debug) { std::cerr << "Distortion violation\n"; }
#endif
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
	// create hypothesis and calculate all its scores
#ifdef DEBUGLATTICE
	if (debug2) { std::cerr << "::EXT: " << transOpt << "\n"; }
#endif
	Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
	newHypo->CalcScore(m_transOptColl->GetFutureScore());
	
	// logging for the curious
	IFVERBOSE(3) {
		const StaticData &staticData = StaticData::Instance();
	  newHypo->PrintHypothesis(m_source
														, staticData.GetWeightDistortion()
														, staticData.GetWeightWordPenalty());
	}

	// add to hypothesis stack
	size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();	
	m_hypoStackColl[wordsTranslated].AddPrune(newHypo);
}

/**
 * Find best hypothesis on the last stack.
 * This is the end point of the best translation, which can be traced back from here
 */
const Hypothesis *Manager::GetBestHypothesis() const
{
	const HypothesisStack &hypoColl = m_hypoStackColl.back();
	return hypoColl.GetBestHypothesis();
}

/**
 * Logging of hypothesis stack sizes
 */
void Manager::OutputHypoStackSize()
{
	std::vector < HypothesisStack >::const_iterator iterStack = m_hypoStackColl.begin();
	TRACE_ERR( "Stack sizes: " << (int)iterStack->size());
	for (++iterStack; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{
		TRACE_ERR( ", " << (int)iterStack->size());
	}
	TRACE_ERR( endl);
}

/**
 * Logging of hypothesis stack contents
 * \param stack number of stack to be reported, report all stacks if 0 
 */
void Manager::OutputHypoStack(int stack)
{
	if (stack >= 0)
	{
		TRACE_ERR( "Stack " << stack << ": " << endl << m_hypoStackColl[stack] << endl);
	}
	else
	{ // all stacks
		int i = 0;
		vector < HypothesisStack >::iterator iterStack;
		for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
		{
			HypothesisStack &hypoColl = *iterStack;
			TRACE_ERR( "Stack " << i++ << ": " << endl << hypoColl << endl);
		}
	}
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

	vector<const Hypothesis*> sortedPureHypo = m_hypoStackColl.back().GetSortedList();

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
			const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
			if (nBestFactor > 0)
				contenders.Prune(count * nBestFactor);
		}
		else
		{
			contenders.Prune(count);
		}
	}
}

void Manager::CalcDecoderStatistics() const 
{
  const Hypothesis *hypo = GetBestHypothesis();
	if (hypo != NULL)
  {
		StaticData::Instance().GetSentenceStats().CalcFinalStats(*hypo);
    IFVERBOSE(2) {
		 	if (hypo != NULL) {
		   	string buff;
		  	string buff2;
		   	TRACE_ERR( "Source and Target Units:"
		 							<< *StaticData::Instance().GetInput());
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
