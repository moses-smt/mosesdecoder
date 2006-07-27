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
#include "LMList.h"

using namespace std;

Manager::Manager(InputType const& source, 
								 TranslationOptionCollection& toc,
								 StaticData &staticData)
:m_source(source)
,m_hypoStack(source.GetSize() + 1)
,m_staticData(staticData)
,m_possibleTranslations(toc)  //dynamic_cast<Sentence const&>(source))
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

	// output
	//TRACE_ERR (m_possibleTranslations << endl);


	// seed hypothesis
	{

		Hypothesis *hypo = Hypothesis::Create(m_source);
#ifdef N_BEST
		LMList allLM = m_staticData.GetAllLM();
		hypo->ResizeComponentScore(allLM, decodeStepList);
#endif
	m_hypoStack[0].AddPrune(hypo);
	}
	
	// go thru each stack
	std::vector < HypothesisCollection >::iterator iterStack;
	for (iterStack = m_hypoStack.begin() ; iterStack != m_hypoStack.end() ; ++iterStack)
	{
		HypothesisCollection &sourceHypoColl = *iterStack;
		sourceHypoColl.PruneToSize(m_staticData.GetMaxHypoStackSize());
		sourceHypoColl.InitializeArcs();
		//sourceHypoColl.Prune();

		ProcessOneStack(sourceHypoColl);

		//OutputHypoStack();
		if (m_staticData.GetVerboseLevel() > 0) {
			OutputHypoStackSize();
		}

	}

    cerr << "Hypotheses created since startup: "<< Hypothesis::s_HypothesesCreated<<endl;

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

void Manager::ProcessOneStack(HypothesisCollection &sourceHypoColl)
{
	// go thru each hypothesis in the stack
	HypothesisCollection::const_iterator iterHypo;
	for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
	{
		Hypothesis &hypothesis = **iterHypo;
		ProcessOneHypothesis(hypothesis);
	}
}

// find all translation options to expand one hypothesis, trigger expansion
// this is mostly a check for overlap with already covered words, and for
// violation of reordering limits.
void Manager::ProcessOneHypothesis(const Hypothesis &hypothesis)
{
	// since we check for reordering limits, its good to have that limit handy
	int maxDistortion = m_staticData.GetMaxDistortion();

	// no limit of reordering: only check for overlap
	if (maxDistortion < 0)
	{	
		TranslationOptionCollection::const_iterator iterTransOpt;
		for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = **iterTransOpt;

			if ( !transOpt.Overlap(hypothesis)) 
			{
				ExpandHypothesis(hypothesis,transOpt);
			}
		}
		return; // done with special case
	}

	// if there are reordering limits, make sure it is not violated
	// the coverage bitmap is handy here (and the position of the first gap)
	const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
	size_t hypoWordCount		= hypoBitmap.GetWordsCount();
	size_t hypoFirstGapPos	= hypoBitmap.GetFirstGapPos();
	
	// MAIN LOOP. go through each possible hypo
	TranslationOptionCollection::const_iterator iterTransOpt;
	for (iterTransOpt = m_possibleTranslations.begin(); iterTransOpt != m_possibleTranslations.end(); ++iterTransOpt)
		{
			const TranslationOption &transOpt = **iterTransOpt;
			// calc distortion if using this poss trans
			
			size_t transOptStartPos = transOpt.GetStartPos();
			
			// no gap so far => don't skip more than allowed limit
			if (hypoFirstGapPos == hypoWordCount)
				{
					if (transOptStartPos == hypoWordCount
							|| (transOptStartPos > hypoWordCount 
									&& transOpt.GetEndPos() <= hypoWordCount + maxDistortion)
					)
				{
					ExpandHypothesis(hypothesis,transOpt);
				}
			}
			// filling in gap => just check for overlap
			else if (transOptStartPos < hypoWordCount)
				{
					if (transOptStartPos >= hypoFirstGapPos
						&& !transOpt.Overlap(hypothesis))
					{
						ExpandHypothesis(hypothesis,transOpt);
					}
				}
			// ignoring, continuing forward => be limited by start of gap
			else
				{
					if (transOpt.GetEndPos() <= hypoFirstGapPos + maxDistortion
						&& !transOpt.Overlap(hypothesis))
					{
						ExpandHypothesis(hypothesis,transOpt);
					}
				}
		}
}

// expand one hypothesis with a translation option
// this involves initial creation, scoring and adding it to the proper stack
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
	if (m_hypoStack[wordsTranslated].AddPrune(newHypo))
		{
			// some additional logging
			if(m_staticData.GetVerboseLevel() > 2) 
				{
					if(m_hypoStack[wordsTranslated].getBestScore() == newHypo->GetScore(ScoreType::Total))
						{
							cout << "new best estimate for this stack" << endl;
						}
					cout << "added hypothesis on stack " << wordsTranslated 
							 << " now size "<<m_hypoStack[wordsTranslated].size() << endl << endl;
				}
		}
	else 
		{
			if(m_staticData.GetVerboseLevel() > 2) 
				{
					cout << "below threshold, discarded" << endl << endl;
				}
		}
	
}


// OLD FUNCTIONS

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
