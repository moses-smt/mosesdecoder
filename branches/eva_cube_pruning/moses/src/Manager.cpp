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
#include "TranslationOptionOrderer.h"
#include "CubePruningData.h"

using namespace std;

#undef DEBUGLATTICE
#ifdef DEBUGLATTICE
static bool debug2 = false;
#endif

// SOME GLOBAL VARIABLES
// set k for cube pruning 
int top_k;
int buffer_size;

typedef set<Hypothesis*, HypothesisScoreOrderer > OrderedHypothesesSet;
typedef map< WordsBitmap, OrderedHypothesesSet > CoverageHypothesesMap;

// store candidates for each stack
map<size_t, CoverageHypothesesMap > candidates; 

// store Hypothesis list and TranslationOption list for each grid
CubePruningData cubePruningData;


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
		sourceHypoColl.SetBeamThreshold(staticData.GetBeamThreshold());
	}
}

Manager::~Manager() 
{
  delete m_transOptColl;
	StaticData::Instance().CleanUpAfterSentenceProcessing();      
	cubePruningData.DeleteData();

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
	const vector <DecodeGraph>
			&decodeStepVL = staticData.GetDecodeStepVL();
	top_k = staticData.GetTopK();
	buffer_size = staticData.GetBufferSize();
	cout << "Cube pruning values: " << top_k << " " << buffer_size << endl; 
		
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
	size_t stack=0;
	std::vector < HypothesisStack >::iterator iterStack;
	
	for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{
//		cout << endl << "STACK " << stack << ":" << endl;
		
		HypothesisStack &sourceHypoColl = *iterStack;
		// the stack is pruned before processing (lazy pruning):
		VERBOSE(3,"processing hypothesis from next stack");
		sourceHypoColl.PruneToSize(staticData.GetMaxHypoStackSize());
		VERBOSE(3,std::endl);
		sourceHypoColl.CleanupArcList();
		
		// keep already seen coverages in mind
		//set<vector<size_t> > seenCoverages; 
		set< WordsBitmap > seenCoverages;
		
		// go through each hypothesis on the stack, find the set of hypothesis with same coverage and expand this set using cube pruning
		HypothesisStack::const_iterator iterHypo;
		for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
		{
				// take first hypothesis from stack to get coverage
			  Hypothesis &hypothesis = **iterHypo;
			
				const WordsBitmap &wb = hypothesis.GetWordsBitmap();
				//vector<size_t> cov = wb.GetCompressedRepresentation();
				
				// check if coverage of current hypothesis was already seen --> if no, proceed
				if( (seenCoverages.count( wb )) == 0 )
				{
					seenCoverages.insert( wb );
					const OrderedHypothesesSet coverageSet = sourceHypoColl.GetCoverageSet( wb );
					vector< Hypothesis*> coverageVec(coverageSet.begin(), coverageSet.end());	
								
					if(coverageVec.size() > 0)
					{
//						cout << endl << "Coverage: " << wb << endl;
						ProcessCoverageVector(coverageVec, wb);
					}
				}
		}
		// do cube pruning for current stack
		CubePruning(stack);
		stack++;
		
		// some logging
		IFVERBOSE(2) {
			OutputHypoStackSize();
		}
	}
	
	// clear global variable
	candidates.clear();

	// some more logging
	VERBOSE(2, staticData.GetSentenceStats());
}



void Manager::ProcessCoverageVector(const vector< Hypothesis*> &coverageVec, const WordsBitmap &hypoBitmap)
{
	// since we check for reordering limits, its good to have that limit handy
	int maxDistortion = StaticData::Instance().GetMaxDistortion();
	bool isWordLattice = StaticData::Instance().GetInputType() == WordLatticeInput;
	// no limit of reordering: only check for overlap
	if (maxDistortion < 0)
	{	
		const size_t hypoFirstGapPos	= hypoBitmap.GetFirstGapPos();
		const size_t sourceSize = m_source.GetSize();

		for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
		{
			// maximal size of new phrase
      		size_t maxSize = sourceSize - startPos;
      		// search for longest possible phrase --> maxSize
      		size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      		maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;

			for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
			{
				if (!hypoBitmap.Overlap(WordsRange(startPos, endPos)))
				{
					// all TranslationOptions returned by 'GetTranslationOptionList' have the same word range
					// 'tol' is of type vector<TranslationOption*>
					TranslationOptionList tol = m_transOptColl->GetTranslationOptionList(WordsRange(startPos, endPos));
					if(tol.size() > 0)
					{	
						PrepareCubePruning(coverageVec, tol);
					}
				}
			}
		}

		return; // done with special case (no reordering limit)
	}
	
	// if there are reordering limits, make sure it is not violated
	// the coverage bitmap is handy here (and the position of the first gap)
	const size_t	hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
							, sourceSize			= m_source.GetSize();
	
	// MAIN LOOP. go through each possible hypo
	for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
	{
    size_t maxSize = sourceSize - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();

    maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;
    
    // check conditions for one hypothesis of the coverage vector
    Hypothesis &hypothesis = *coverageVec[0];

		for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
		{
			// check for overlap
		  WordsRange extRange(startPos, endPos);
			if (hypoBitmap.Overlap(extRange) ||
			      (isWordLattice && (!m_source.IsCoveragePossible(extRange) ||
					                     !m_source.IsExtensionPossible(hypothesis.GetCurrSourceWordsRange(), extRange))
					  )
			   )
		  {
			  continue;
			}
			
			bool leftMostEdge = (hypoFirstGapPos == startPos);
			
			// any length extension is okay if starting at left-most edge
			if (leftMostEdge)
			{
				TranslationOptionList tol = m_transOptColl->GetTranslationOptionList(extRange);
				if(tol.size() > 0)
				{	
					PrepareCubePruning(coverageVec, tol);
				}
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
					
					TranslationOptionList tol = m_transOptColl->GetTranslationOptionList(extRange);
					if(tol.size() > 0)
					{	
						PrepareCubePruning(coverageVec, tol);
					}
				}
			}
		}
	}
}

void Manager::PrepareCubePruning(const vector< Hypothesis*> &coverageVec, TranslationOptionList &tol)
{
	set<TranslationOption*, TranslationOptionOrderer> translationOptionSet;
	translationOptionSet.insert(tol.begin(), tol.end());
	
	// translation options are ordered now
	TranslationOptionList orderedTol( translationOptionSet.begin(), translationOptionSet.end() );
   				
  // initialize cand with the hypothesis 1,1
	Hypothesis *newHypo = (coverageVec[0])->CreateNext(*orderedTol[0]);
	newHypo->CalcScore(m_transOptColl->GetFutureScore());
	newHypo->SetGridPosition(0, 0);
	candidates[ coverageVec[0]->GetWordsBitmap().GetNumWordsCovered() ][newHypo->GetWordsBitmap()].insert(newHypo);
	
	// store information for this hypothesis and coverage in CubePruningData object
	cubePruningData.SaveData(newHypo, coverageVec, orderedTol);
}

void Manager::CubePruning(size_t stack)
{	
////	cout << "\n\nCUBE PRUNING FOR STACK " << stack << endl;
  CoverageHypothesesMap candsForStack = candidates[stack];
  CoverageHypothesesMap::iterator cands_iter;
  for(cands_iter = candsForStack.begin(); cands_iter != candsForStack.end(); ++cands_iter)
  {
  	WordsBitmap wb = (*cands_iter).first;
 //// 	cout << "\nCubePruning for coverage " << wb << endl;
  	OrderedHypothesesSet cand = (*cands_iter).second;
		
		vector< Hypothesis*> coverageVec; 
  	TranslationOptionList tol;
					
		Hypothesis *item, *newHypo;
		// KBEST
		// ".. enumerating the consequent items best-first while keeping track of a relatively small
		//  number of candidates [..] for the next best item." 
		// "When we take into account the combination costs, the grid is no longer monotonic in general.."
		// "Because of this disordering, we do not put the enumerated items direcly into D(v); instead,
		//  we collect items in a buffer.."
  	OrderedHypothesesSet buf;  
  
		size_t x = 0, y = 0;
	
		map< WordsBitmap, map< size_t, list<size_t> > > tickedOff;
		
	 	while( !(cand.empty()) && ((buffer_size == -1) || (buf.size() < buffer_size)) )
	  {
  		// "The heart of the algorithm is lines 10-12. Lines 10-11 move the best derivation [..] from cand to buf, 
  		// and then line 12 pushes its successors [..] into cand." 
  		// 10: POP-MIN(cand); 11: append item to buf; 12: PUSHSUCC(item, cand);
  		
  		item = *(cand.begin());  		
  		
  		coverageVec = (cubePruningData.xData)[item->GetId()];
  		tol = (cubePruningData.yData)[item->GetId()];
  		// If the search is performed in more than one grid, the grids have to be differentiated by the coverage
	  	// on the left side of the grid.
	    WordsBitmap grid_wb = (*coverageVec.begin())->GetWordsBitmap(); 
  	
  		// update grid position
  		x = item->GetXGridPosition();
  		y = item->GetYGridPosition();
	  	buf.insert(item);
	  	// tick off hypothesis
	  	tickedOff[grid_wb][x].push_back(y);
	  	cand.erase(cand.begin());
	  	
	  	// PUSHSUCC(item, cand); --> insert neighbours of item into cand
	  	// neighbour on the right side, same hypothesis, new extension
	  	
	  	// Check if there are more options in the grid and if hypothesis 'newHypo' was not already created before
	  	if( (coverageVec.size() > x) && (tol.size() > y+1) && !IsTickedOff(tickedOff, grid_wb, x, y+1) )
	  	{ 
				newHypo = (coverageVec[x])->CreateNext(*tol[y+1]);
				newHypo->CalcScore(m_transOptColl->GetFutureScore());
				newHypo->SetGridPosition(x, y+1);
  			cand.insert( newHypo );
  			// tick off hypothesis
  			tickedOff[grid_wb][x].push_back(y+1);
  			// TODO: this is not efficient because the same information is stored for different hypotheses in the same grid
				cubePruningData.SaveData(newHypo, coverageVec,tol);				
  		}
  		// neighbour below, new hypothesis, same extension
  		if( (coverageVec.size() > x+1) && (tol.size() > y) && !IsTickedOff(tickedOff, grid_wb, x+1, y) )
  		{
				newHypo = (coverageVec[x+1])->CreateNext(*tol[y]);
				newHypo->CalcScore(m_transOptColl->GetFutureScore());
				newHypo->SetGridPosition(x+1, y);
   			cand.insert( newHypo );
   			// tick off hypothesis
   			tickedOff[grid_wb][x+1].push_back(y);
   			// TODO: this is not efficient because the same information is stored for different hypotheses in the same grid
				cubePruningData.SaveData(newHypo, coverageVec,tol);
   		}
  	}
  	// delete elements in cand that were not transferred to buffer
		RemoveAllInColl(cand);
		  	
  	// "Re-sort the buffer into D(v) after it has accumulated k items."
  	// --> chose top_k items in buffer and add to stacks 
  	set<Hypothesis*, HypothesisScoreOrderer >::iterator buf_iter;
  	size_t i=0;
  	// add top_k hypothesis to hypothesis stack
  	for(buf_iter = buf.begin(); buf_iter != buf.end(); ++buf_iter)
  	{
			if((top_k == -1) || (i < top_k))
			{
				Hypothesis *newHypo = *buf_iter;
				size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();
					
				// logging for the curious
	IFVERBOSE(3) {
		const StaticData &staticData = StaticData::Instance();
	  newHypo->PrintHypothesis(m_source
														, staticData.GetWeightDistortion()
														, staticData.GetWeightWordPenalty());
	}	
				m_hypoStackColl[wordsTranslated].AddPrune(newHypo);
			}
   		else
	   		delete *buf_iter;
    	i++;
  	}
  	buf.clear();
  }
}

/**
 *  Check if this certain point of the grid (x, y) has already been processed;
 *  If yes, it should be ticked off
 */
bool Manager::IsTickedOff(map< WordsBitmap, map< size_t, list<size_t> > > &tickedOff, WordsBitmap &wb, size_t x, size_t y)
{
	list<size_t> l = tickedOff[wb][x];
	if( find(l.begin(), l.end(), y) == l.end() )
		return false;
	else
		return true;
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
