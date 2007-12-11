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

// set k for cube pruning 
const size_t top_k = 40;

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
	
	// store Cube Pruning information in CubePruningData object
	CubePruningData cubePruningData;
	
	for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{
		cout << endl << "STACK " << stack << ":" << endl;
		
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
				Hypothesis *hypothesis = *iterHypo;
				const WordsBitmap &wb = hypothesis->GetWordsBitmap();
				//vector<size_t> cov = wb.GetCompressedRepresentation();
				
				// check if coverage of current hypothesis was already seen --> if no, proceed
				if( (seenCoverages.count( wb )) == 0 )
				{
					seenCoverages.insert( wb );
					const set<Hypothesis*, HypothesisScoreOrderer> coverageSet = sourceHypoColl.GetCoverageSet( wb );
					vector< Hypothesis*> coverageVec(coverageSet.begin(), coverageSet.end());	
								
					if(coverageVec.size() > 0)
					{
						cout << endl << "Coverage: " << wb << endl;
						ProcessCoverageVector(coverageVec, wb, cubePruningData);
					}
				}
		}
		// do cube pruning for current stack
		cout << "--> CubePruning" << endl << endl;
		CubePruning(stack, cubePruningData);
		stack++;
		// some logging
		OutputHypoStackSize();
	}

	// some more logging
	VERBOSE(2, staticData.GetSentenceStats());
}



void Manager::ProcessCoverageVector(const vector< Hypothesis*> &coverageVec, const WordsBitmap &hypoBitmap, CubePruningData &cubePruningData)
{
	Hypothesis *best;
	TranslationOptionList bestTol;
	
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
						PrepareCubePruning(coverageVec, tol, cubePruningData);
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
    Hypothesis *hypothesis = coverageVec[0];

		for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
		{
			// check for overlap
		  WordsRange extRange(startPos, endPos);
			if (hypoBitmap.Overlap(extRange) ||
			      (isWordLattice && (!m_source.IsCoveragePossible(extRange) ||
					                     !m_source.IsExtensionPossible(hypothesis->GetCurrSourceWordsRange(), extRange))
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
					PrepareCubePruning(coverageVec, tol, cubePruningData);
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
						PrepareCubePruning(coverageVec, tol, cubePruningData);
					}
				}
			}
		}
	}
}

void Manager::PrepareCubePruning(const vector< Hypothesis*> &coverageVec, TranslationOptionList &tol, CubePruningData &cubePruningData)
{
//	cout << "PrepareCubePruning" << endl;
	set<TranslationOption*, TranslationOptionOrderer> translationOptionSet;
	translationOptionSet.insert(tol.begin(), tol.end());
	
	// translation options are ordered now
	TranslationOptionList orderedTol( translationOptionSet.begin(), translationOptionSet.end() );
   				
  // initialize cand with the hypothesis 1,1
  Hypothesis *newHypo = (coverageVec[0])->CreateNext(*orderedTol[0]);
	newHypo->CalcScore(m_transOptColl->GetFutureScore());
	newHypo->SetGridPosition(0, 0);
	(cubePruningData.cand)[ coverageVec[0]->GetWordsBitmap().GetNumWordsCovered() ].insert(newHypo);
//	cout << "\tinsert into cand " << coverageVec[0]->GetWordsBitmap().GetNumWordsCovered() << ": " << newHypo->GetTotalScore() << "  cov: " << coverageVec[0]->GetWordsBitmap() << "  tol: " << orderedTol[0]->GetSourceWordsRange() << endl;
	
	// save data for this coverage
	cout << "SAVE: " << newHypo->GetWordsBitmap() << " cov: " << coverageVec.size() << "  tol: " << orderedTol.size() << endl;
	cubePruningData.SaveData(newHypo->GetWordsBitmap(), coverageVec, orderedTol);
}

void Manager::CubePruning(size_t stack, CubePruningData &cubePruningData)
{	
	vector< Hypothesis*> coverageVec; 
  TranslationOptionList tol;
   					
  set<Hypothesis*, HypothesisScoreOrderer >::iterator cand_iter;
   					
	Hypothesis *item, *newHypo;
	// KBEST
	// ".. enumerating the consequent items best-first while keeping track of a relatively small
	//  number of candidates [..] for the next best item." 
	// "When we take into account the combination costs, the grid is no longer monotonic in general.."
	// "Because of this disordering, we do not put the enumerated items direcly into D(v); instead,
	//  we collect items in a buffer.."
  set<Hypothesis*, HypothesisScoreOrderer > D, buf;
  set<Hypothesis*, HypothesisScoreOrderer > &cand = (cubePruningData.cand)[stack];
	size_t x = 0, y = 0;
	cout << "size of cand: " << cand.size() << endl;
 	while( !(cand.empty()) && (buf.size() < top_k) )
  {
  	IFVERBOSE(3) {
   		cout << "candidates: " << endl;
   		for(cand_iter = cand.begin(); cand_iter != cand.end(); ++cand_iter)
   		{
   			cout << (*cand_iter)->GetXGridPosition() << " " << (*cand_iter)->GetYGridPosition() << "   " << (*cand_iter)->GetTotalScore() << endl;
   		}
   		cout << endl; 
   	}	
  	// "The heart of the algorithm is lines 10-12. Lines 10-11 move the best derivation [..] from cand to buf, 
  	// and then line 12 pushes its successors [..] into cand." 
  	// 10: POP-MIN(cand); 11: append item to buf; 12: PUSHSUCC(item, cand);
  	item = *(cand.begin());  		
  	coverageVec = (cubePruningData.xData)[item->GetWordsBitmap()];
  	tol = (cubePruningData.yData)[item->GetWordsBitmap()];
  	
  	cout << "item bitmap: " << item->GetWordsBitmap() << endl;
  	if(coverageVec.size() > 0)
  		cout << "coverageVec: " << coverageVec[0]->GetWordsBitmap() << endl;
  	else
  		cout << endl;
  	if(tol.size() > 0)
  		cout << "tol: " << tol[0]->GetSourceWordsRange() << endl << endl;
  	else
  		cout << endl;
  	
  	// update grid position
  	x = item->GetXGridPosition();
  	y = item->GetYGridPosition();
	//cout << "chosen cand: " << x << " " << y << endl;
	  buf.insert(item);
	  cand.erase(cand.begin());
	  // PUSHSUCC(item, cand); --> insert neighbours of item into cand
	  // neighbour on the right side, same hypothesis, new extension
	  if( (coverageVec.size() > x) && (tol.size() > y+1) )
	  { 
  		newHypo = (coverageVec[x])->CreateNext(*tol[y+1]);
			newHypo->CalcScore(m_transOptColl->GetFutureScore());
			newHypo->SetGridPosition(x, y+1);
  		cand.insert( newHypo );
  		// logging for the curious
			IFVERBOSE(3) {
				const StaticData &staticData = StaticData::Instance();
				newHypo->PrintHypothesis(m_source
						, staticData.GetWeightDistortion()
						, staticData.GetWeightWordPenalty());
			}
  	}
  	// neighbour below, new hypothesis, same extension
  	if( (coverageVec.size() > x+1) && (tol.size() > y) )
  	{
	  	newHypo = (coverageVec[x+1])->CreateNext(*tol[y]);
			newHypo->CalcScore(m_transOptColl->GetFutureScore());
			newHypo->SetGridPosition(x+1, y);
   		cand.insert( newHypo );
   		// logging for the curious
			IFVERBOSE(3) {
				const StaticData &staticData = StaticData::Instance();
		  	newHypo->PrintHypothesis(m_source
										, staticData.GetWeightDistortion()
										, staticData.GetWeightWordPenalty());
			}
   	}
  }
  // "Re-sort the buffer into D(v) after it has accumulated k items."
  // buffer has an ordering function, just copy to D or if buf is larger than D, copy the top_k items of buf to D
  set<Hypothesis*, HypothesisScoreOrderer >::iterator buf_iter;
  size_t l=0;
  for(buf_iter = buf.begin(); buf_iter != buf.end(); ++buf_iter)
  {
  	if(l < top_k)
   		D.insert(*buf_iter);
   	else
   		break;
    l++;
  }
  // add all hypothesis in D to hypothesis stack
  cout << "size D: " << D.size() << endl << endl;
  set<Hypothesis*, HypothesisScoreOrderer >::iterator d_iter;
  for(d_iter = D.begin(); d_iter != D.end(); ++d_iter)
  {
  	Hypothesis *newHypo = *d_iter;
  	size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();	
		m_hypoStackColl[wordsTranslated].AddPrune(newHypo);
  }
  cout << endl;
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
