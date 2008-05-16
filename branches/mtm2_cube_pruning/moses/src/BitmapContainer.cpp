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

////////////////////////////////////////////////////////////////////////////////
// BackwardsEdge Code
////////////////////////////////////////////////////////////////////////////////

BackwardsEdge::BackwardsEdge(const BitmapContainer &prevBitmapContainer
							 , BitmapContainer &parent
							 , const TranslationOptionList &translations
							 , const SquareMatrix &futureScore
							 , const size_t kBestCubePruning)
  : m_initialized(false)
  , m_prevBitmapContainer(prevBitmapContainer)
  , m_parent(parent)
  , m_kbest_translations(translations)
  , m_futurescore(futureScore)
  , m_kbest(kBestCubePruning)
  , m_seenPosition()
{

	// If either dimension is empty, we haven't got anything to do.
	if(m_prevBitmapContainer.GetHypotheses().size() == 0 || m_kbest_translations.size() == 0) {
		VERBOSE(3, "Empty cube on BackwardsEdge" << std::endl);
		m_hypothesis_maxpos = 0;
		m_translations_maxpos = 0;
		return;
	}

	// Fetch the things we need for distortion cost computation.
	int maxDistortion = StaticData::Instance().GetMaxDistortion();

	if (maxDistortion == -1) {
		for (HypothesisSet::const_iterator iter = m_prevBitmapContainer.GetHypotheses().begin(); iter != m_prevBitmapContainer.GetHypotheses().end(); ++iter)
			{
				m_kbest_hypotheses.push_back(*iter);
			}
		m_hypothesis_maxpos = m_kbest_hypotheses.size();
		m_translations_maxpos = m_kbest_translations.size();
		return;
	}

	WordsRange transOptRange = translations[0]->GetSourceWordsRange();
	const InputType *itype = StaticData::Instance().GetInput();

	for (HypothesisSet::const_iterator iter = m_prevBitmapContainer.GetHypotheses().begin(); iter != m_prevBitmapContainer.GetHypotheses().end(); ++iter) {
		// Special case: If this is the first hypothesis used to seed the search,
		// it doesn't have a valid range, and we create the hypothesis, if the
		// initial position is not further into the sentence than the distortion limit.
		if ((*iter)->GetWordsBitmap().GetNumWordsCovered() == 0)
			{
				if (transOptRange.GetStartPos() <= maxDistortion)
					m_kbest_hypotheses.push_back(*iter);
			}
		else
			{
				int distortionDistance = itype->ComputeDistortionDistance((*iter)->GetCurrSourceWordsRange()
																		, transOptRange);

				if (distortionDistance <= maxDistortion)
					m_kbest_hypotheses.push_back(*iter);
			}
	}

	if (m_kbest_translations.size() > 1)
	{
		assert(m_kbest_translations[0]->GetFutureScore() >= m_kbest_translations[1]->GetFutureScore());
	}

	if (m_kbest_hypotheses.size() > 1)
	{
		std::cerr << *m_kbest_hypotheses[0] << std::endl;
		std::cerr << *m_kbest_hypotheses[1] << std::endl;
		assert(m_kbest_hypotheses[0]->GetTotalScore() >= m_kbest_hypotheses[1]->GetTotalScore());
	}	

	m_hypothesis_maxpos = m_kbest_hypotheses.size();
	m_translations_maxpos = m_kbest_translations.size();
}

BackwardsEdge::~BackwardsEdge()
{
	m_seenPosition.clear();
	m_kbest_hypotheses.clear();
}


void
BackwardsEdge::Initialize()
{
	if(m_hypothesis_maxpos == 0 || m_translations_maxpos == 0)
	{
		m_initialized = true;
		return;
	}

	Hypothesis *expanded = CreateHypothesis(*m_kbest_hypotheses[0], *m_kbest_translations[0]);
	m_parent.Enqueue(0, 0, expanded, this);
	m_seenPosition.insert(0);
	m_initialized = true;
}

Hypothesis *BackwardsEdge::CreateHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt) 
{
	// create hypothesis and calculate all its scores
	Hypothesis *newHypo = hypothesis.CreateNext(transOpt);

/*
	// expand hypothesis further if transOpt was linked
	for (std::vector<TranslationOption*>::const_iterator iterLinked = transOpt.GetLinkedTransOpts().begin();
		 iterLinked != transOpt.GetLinkedTransOpts().end(); ++iterLinked) {
		const WordsBitmap hypoBitmap = newHypo->GetWordsBitmap();
		if (hypoBitmap.Overlap((**iterLinked).GetSourceWordsRange())) {
			// don't want to add a hypothesis that has some but not all of a linked TO set, so return
			delete newHypo;
			return NULL;
		}
		else
		{
			newHypo->CalcScore(m_futurescore);
			newHypo = newHypo->CreateNext(**iterLinked);
		}
	}
*/

	newHypo->CalcScore(m_futurescore);
	
	return newHypo;
}

bool
BackwardsEdge::SeenPosition(int x, int y)
{
	std::set< int >::iterator iter = m_seenPosition.find(m_kbest * x + y);
	return (iter != m_seenPosition.end());
}


bool
BackwardsEdge::GetInitialized()
{
	return m_initialized;
}

const BitmapContainer&
BackwardsEdge::GetBitmapContainer() const
{
	return m_prevBitmapContainer;
}

void
BackwardsEdge::PushSuccessors(int x, int y)
{
	Hypothesis *newHypo;
	
	if(y + 1 < m_translations_maxpos && !SeenPosition(x, y + 1)) {
		newHypo = CreateHypothesis(*m_kbest_hypotheses[x], *m_kbest_translations[y + 1]);

		if(newHypo != NULL)
		{
			m_parent.Enqueue(x, y + 1, newHypo, (BackwardsEdge*)this);
			m_seenPosition.insert(m_kbest * x + y);
		}
	}

	if(x + 1 < m_hypothesis_maxpos && !SeenPosition(x + 1, y)) {
		newHypo = CreateHypothesis(*m_kbest_hypotheses[x + 1], *m_kbest_translations[y]);

		if(newHypo != NULL)
		{
			m_parent.Enqueue(x + 1, y, newHypo, (BackwardsEdge*)this);
			m_seenPosition.insert(m_kbest * x + y);
		}
	}	
}


////////////////////////////////////////////////////////////////////////////////
// BitmapContainer Code
////////////////////////////////////////////////////////////////////////////////

BitmapContainer::BitmapContainer(const WordsBitmap &bitmap
																 , HypothesisStack &stack
																 , const size_t KBestCubePruning)
  : m_bitmap(bitmap)
  , m_stack(stack)
  , m_kbest(KBestCubePruning)
{
	m_hypotheses = HypothesisSet();
	m_edges = BackwardsEdgeSet();
	m_queue = HypothesisQueue();
}

BitmapContainer::~BitmapContainer()
{
	// As we have created the square position objects we clean up now.
	while (!m_queue.empty())
	{
		HypothesisQueueItem *ret = m_queue.top();
		FREEHYPO(ret->GetHypothesis());
		delete ret;
		m_queue.pop();
	}

	// Delete all edges.
	RemoveAllInColl(m_edges);

	m_hypotheses.clear();
	m_edges.clear();
}


void
BitmapContainer::Enqueue(int hypothesis_pos
												 , int translation_pos
												 , Hypothesis *hypothesis
												 , BackwardsEdge *edge)
{
	HypothesisQueueItem *item = new HypothesisQueueItem(hypothesis_pos
																										  , translation_pos
																										  , hypothesis
																											, edge);
	if (m_queue.size() > 0)
		std::cerr << "OLD_TOP = " << m_queue.top()->GetHypothesis()->GetTotalScore() << std::endl;
	m_queue.push(item);
	if (m_queue.size() > 1)
		std::cerr << "NEW_TOP = " << m_queue.top()->GetHypothesis()->GetTotalScore() << std::endl;
}

HypothesisQueueItem*
BitmapContainer::Dequeue(bool keepValue)
{
	if (!m_queue.empty())
	{
		HypothesisQueueItem *item = m_queue.top();

		if (!keepValue)
		{
			m_queue.pop();
		}

		return item;
	}

	return NULL;
}

HypothesisQueueItem*
BitmapContainer::Top()
{
	return m_queue.top();
}

size_t
BitmapContainer::Size()
{
	return m_queue.size();
}

bool
BitmapContainer::Empty()
{
	return m_queue.empty();
}


const WordsBitmap&
BitmapContainer::GetWordsBitmap()
{
	return m_bitmap;
}

const HypothesisSet&
BitmapContainer::GetHypotheses() const
{
	return m_hypotheses;
}

size_t
BitmapContainer::GetHypothesesSize() const
{
	return m_hypotheses.size();
}

const BackwardsEdgeSet&
BitmapContainer::GetBackwardsEdges()
{
	return m_edges;
}

void
BitmapContainer::AddHypothesis(Hypothesis *hypothesis)
{
	bool itemExists = false;
	HypothesisSet::const_iterator iter;
	for (iter = m_hypotheses.begin(); iter != m_hypotheses.end(); ++iter)
	{
		if (*iter == hypothesis) {
			itemExists = true;
			break;
		}
	}
	assert(itemExists == false);
	m_hypotheses.push_back(hypothesis);
}

void
BitmapContainer::AddBackwardsEdge(BackwardsEdge *edge)
{
	m_edges.insert(edge);
}

void
BitmapContainer::FindKBestHypotheses()
{
	BackwardsEdgeSet::iterator iter;
	for (iter = m_edges.begin(); iter != m_edges.end(); ++iter)
	{
		BackwardsEdge *edge = *iter;
		edge->Initialize();
	}

	size_t stacked = 0;
	while (stacked < m_kbest)
	{
		if (m_queue.empty())
		{
			return;
		}

		// Get the currently best hypothesis from the queue.
		HypothesisQueueItem *item = Dequeue();
		

		// If the priority queue is exhausted, we are done and should have exited
		assert(item != NULL);
		if (!Empty())
		{
			HypothesisQueueItem *check = Dequeue(true);
			std::cerr << *item->GetHypothesis() << std::endl;
			std::cerr << *check->GetHypothesis() << std::endl;
			assert(item->GetHypothesis()->GetTotalScore() >= check->GetHypothesis()->GetTotalScore());
		}

		// Logging for the criminally insane
		IFVERBOSE(3) {
			const StaticData &staticData = StaticData::Instance();
			item->GetHypothesis()->PrintHypothesis();
		}

		// Add best hypothesis to hypothesis stack.
		bool added = m_stack.AddPrune(item->GetHypothesis());	
		if (added)
		{
			stacked++;
		}

		IFVERBOSE(3) {
			TRACE_ERR("added flag is " << added << std::endl);
		}

		// Create new hypotheses for the two successors of the hypothesis just added.
		int hypothesis_pos = item->GetHypothesisPos();
		int translation_pos = item->GetTranslationPos();
		item->GetBackwardsEdge()->PushSuccessors(hypothesis_pos, translation_pos);

		// We are done with the queue item, we delete it.
		delete item;
	}
}

void
BitmapContainer::SortHypotheses()
{
	std::sort(m_hypotheses.begin(), m_hypotheses.end(), HypothesisScoreOrderer());
}
