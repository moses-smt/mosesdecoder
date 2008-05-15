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

#include <algorithm>
#include <limits>
#include <utility>
#include "BitmapContainer.h"
#include "HypothesisStack.h"

SquarePosition *BackwardsEdge::m_invalid = NULL;

BackwardsEdge::BackwardsEdge(const BitmapContainer &prevBitmapContainer
                             , const TranslationOptionList &translations
							 , const SquareMatrix &futureScore
							 , const size_t KBestCubePruning)
  : m_prevBitmapContainer(prevBitmapContainer)
	, m_queue()
	, m_seenPosition(KBestCubePruning * KBestCubePruning, false)
	, m_initialized(false)
	, m_futurescore(futureScore)
	, m_kbest(KBestCubePruning)
	, m_kbest_translations(translations)
{
	// Copy hypotheses from ordered set to vector for faster access.
	const OrderedHypothesisSet &hypotheses = m_prevBitmapContainer.GetHypotheses();
	
	// If either dimension is empty, we haven't got anything to do.
	if(translations.size() == 0 || hypotheses.size() == 0) {
		VERBOSE(3, "Empty cube on BackwardsEdge" << std::endl);
		m_xmax = 0;
		m_ymax = 0;
		return;
	}
	
	m_ymax = std::min(m_kbest, translations.size());

	m_kbest_hypotheses.resize(hypotheses.size());

	// Fetch the things we need for distortion cost computation
	int maxDistortion = StaticData::Instance().GetMaxDistortion();
	const InputType *itype = StaticData::Instance().GetInput();
	WordsRange transOptRange = translations[0]->GetSourceWordsRange();

	m_xmax = 0;
	OrderedHypothesisSet::const_iterator hypoIter = hypotheses.begin();
	for (; m_xmax < m_kbest && hypoIter != hypotheses.end(); ++hypoIter) 
	{
		// If the combination of this hypothesis and our translation
		// options violates the distortion limit, discard the hypothesis,
		// otherwise store it and increment m_xmax
		
		Hypothesis *current = *hypoIter;
		if (maxDistortion == -1) 
			m_kbest_hypotheses[m_xmax++] = current;
		else {
		  int distortionDistance = itype->ComputeDistortionDistance(current->GetCurrSourceWordsRange(),
														  transOptRange);
		  if (distortionDistance <= maxDistortion)
			m_kbest_hypotheses[m_xmax++] = current;
		}
	}
	
	// Maybe the list has shrunk.
	m_kbest_hypotheses.resize(m_xmax);
}

const SquarePosition
BackwardsEdge::InvalidSquarePosition()
{
	if (BackwardsEdge::m_invalid == NULL) {
		BackwardsEdge::m_invalid = new SquarePosition(NULL, make_pair(-1, -1));
	}

	return *BackwardsEdge::m_invalid;
}

BackwardsEdge::~BackwardsEdge()
{
	// As we have created the square position objects we clean up now.
	for (size_t q_iter = 0; q_iter < m_queue.size(); q_iter++)
	{
		SquarePosition *ret = m_queue.top();
		FREEHYPO(ret->first);
		delete ret;
		m_queue.pop();
	}
}

const BitmapContainer&
BackwardsEdge::GetBitmapContainer() const
{
	return m_prevBitmapContainer;
}

bool
BackwardsEdge::GetInitialized()
{
	return m_initialized;
}

void
BackwardsEdge::Initialize()
{
	if(m_xmax == 0 || m_ymax == 0)
	{
		m_initialized = true;
		return;
	}
	
	Hypothesis *expanded = CreateHypothesis(*m_kbest_hypotheses[0], *m_kbest_translations[0]);
	Enqueue(0, 0, expanded);
	m_seenPosition[0] = true;
	m_initialized = true;
}

void
BackwardsEdge::PushSuccessors(int x, int y)
{
	Hypothesis *newHypo;
	
	if(y + 1 < m_ymax && !SeenPosition(x, y + 1)) {
		newHypo = CreateHypothesis(*m_kbest_hypotheses[x], *m_kbest_translations[y + 1]);
		if(newHypo != NULL)
			Enqueue(x, y + 1, newHypo);
	}
	if(x + 1 < m_xmax && !SeenPosition(x + 1, y)) {
		newHypo = CreateHypothesis(*m_kbest_hypotheses[x + 1], *m_kbest_translations[y]);
		if(newHypo != NULL)
			Enqueue(x + 1, y, newHypo);
	}	
}

/**
* Create one hypothesis with a translation option.
 * this involves initial creation and scoring
 * \param hypothesis hypothesis to be expanded upon
 * \param transOpt translation option (phrase translation) 
 *        that is applied to create the new hypothesis
 */
Hypothesis *BackwardsEdge::CreateHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt) 
{
	// create hypothesis and calculate all its scores
	Hypothesis *newHypo = hypothesis.CreateNext(transOpt);
	// expand hypothesis further if transOpt was linked
	for (std::vector<TranslationOption*>::const_iterator iterLinked = transOpt.GetLinkedTransOpts().begin();
		 iterLinked != transOpt.GetLinkedTransOpts().end(); iterLinked++) {
		const WordsBitmap hypoBitmap = newHypo->GetWordsBitmap();
		if (hypoBitmap.Overlap((**iterLinked).GetSourceWordsRange())) {
			// don't want to add a hypothesis that has some but not all of a linked TO set, so return
			return NULL;
		}
		else
		{
			newHypo->CalcScore(m_futurescore);
			newHypo = newHypo->CreateNext(**iterLinked);
		}
	}
	newHypo->CalcScore(m_futurescore);
	
	return newHypo;
}

void
BackwardsEdge::Enqueue(int x, int y, Hypothesis *hypothesis)
{
	// We create a new square position object with the given values here.
	SquarePosition *chunk = new SquarePosition();
	chunk->first = hypothesis;
	chunk->second = make_pair(x, y);

	// And put it into the priority queue.
	m_queue.push(chunk);
	m_seenPosition[m_kbest * x + y] = true;
}

bool
BackwardsEdge::Empty()
{
	return m_queue.empty();
}

size_t
BackwardsEdge::Size()
{
	return m_queue.size();
}

SquarePosition
BackwardsEdge::Dequeue(bool keepValue)
{
	if (!m_initialized) {
		Initialize();
	}

	// Return the topmost square position object from the priority queue.
	// If keepValue is false (= default), this will remove the topmost object afterwards.
	if (!m_queue.empty()) {
		SquarePosition ret = *(m_queue.top());

		if (!keepValue) {
			m_queue.pop();
		}

		return ret;
	}
	
	return InvalidSquarePosition();
}

bool
BackwardsEdge::SeenPosition(int x, int y)
{
	return m_seenPosition[m_kbest * x + y];
}

BitmapContainer::BitmapContainer(const WordsBitmap &bitmap
								 , HypothesisStack &stack
								 , const size_t KBestCubePruning)
  : m_bitmap(bitmap)	
	, m_stack(stack)
	, m_kbest(KBestCubePruning)
{
	m_hypotheses = OrderedHypothesisSet();
	m_edges = BackwardsEdgeSet();
}

BitmapContainer::~BitmapContainer()
{
	RemoveAllInColl(m_edges);
	m_hypotheses.clear();
}
		
const WordsBitmap&
BitmapContainer::GetWordsBitmap()
{
	return m_bitmap;
}

const OrderedHypothesisSet&
BitmapContainer::GetHypotheses() const
{
	return m_hypotheses;
}

const BackwardsEdgeSet&
BitmapContainer::GetBackwardsEdges()
{
	return m_edges;
}

void
BitmapContainer::AddHypothesis(Hypothesis *hypothesis)
{
	OrderedHypothesisSet::const_iterator iter = m_hypotheses.find(hypothesis);
	assert(iter == m_hypotheses.end());
	m_hypotheses.insert(hypothesis);
}

void
BitmapContainer::AddBackwardsEdge(BackwardsEdge *edge)
{
	m_edges.insert(edge);
}

void
BitmapContainer::FindKBestHypotheses()
{
	// find k best translation options
	// repeat k times:
	// 1. iterate over all edges
	// 2. take best hypothesis, expand it
	// 3. put the two successors of the hypothesis on the queue
	//    - add lm scoring to both
	// IMPORTANT: we assume that all scores in the queue are LM-SCORED
	for(size_t i = 0; i < m_kbest; i++) {
		if(m_edges.empty())
			return;
		
		BackwardsEdgeSet::iterator edgeIter = m_edges.begin();
		BackwardsEdge *bestEdge = NULL;
		float bestScore = -std::numeric_limits<float>::infinity();
		while (edgeIter != m_edges.end())
		{
			SquarePosition current = (*edgeIter)->Dequeue(true);
			
			// if the priority queue is exhausted, remove the edge from the set
			// and proceed with the next edge
			if(current == (*edgeIter)->InvalidSquarePosition()) {
				BackwardsEdgeSet::iterator deleteIter = edgeIter++;
				delete *deleteIter;
				m_edges.erase(deleteIter);
				continue;
			}
			
			if(current.first->GetTotalScore() > bestScore) {
				bestScore = current.first->GetTotalScore();
				bestEdge = *edgeIter;
			}

			++edgeIter;
		}
		
		// if all the queues were empty, return
		if(bestEdge == NULL)
			return;
		
		SquarePosition pos = bestEdge->Dequeue(false);
		
		Hypothesis *bestHypo = pos.first;
		
		/*
		 // logging for the curious
		 IFVERBOSE(3) {
			 const StaticData &staticData = StaticData::Instance();
			 bestHypo->PrintHypothesis(bestHypo
									 , staticData.GetWeightDistortion()
									 , staticData.GetWeightWordPenalty());
		 }
		*/
		
		// add to hypothesis stack
		// size_t wordsTranslated = bestHypo->GetWordsBitmap().GetNumWordsCovered();	
		m_stack.AddPrune(bestHypo);	
		
		// create new hypotheses for the two successors of the hypothesis just added
		// and insert them into the priority queue
		int x = pos.second.first;
		int y = pos.second.second;
		
		bestEdge->PushSuccessors(x, y);
	}
}
