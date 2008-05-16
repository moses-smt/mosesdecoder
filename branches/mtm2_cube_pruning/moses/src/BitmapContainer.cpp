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
							 , const size_t KBestCubePruning)
  : m_initialized(false)
  , m_prevBitmapContainer(prevBitmapContainer)
  , m_parent(parent)
  , m_kbest_translations(translations)
  , m_futurescore(futureScore)
  , m_kbest(KBestCubePruning)
  , m_seenPosition()
{
	// Copy hypotheses from ordered set to vector for faster access.
	const OrderedHypothesisSet &hypotheses = m_prevBitmapContainer.GetHypotheses();
	
	// If either dimension is empty, we haven't got anything to do.
	if(translations.size() == 0 || hypotheses.size() == 0) {
		VERBOSE(3, "Empty cube on BackwardsEdge" << std::endl);
		m_hypothesis_maxpos = 0;
		m_translations_maxpos = 0;
		return;
	}
	
	m_hypothesis_maxpos = std::min(m_kbest, hypotheses.size());
	m_translations_maxpos = std::min(m_kbest, translations.size());
	m_kbest_hypotheses.resize(hypotheses.size());

	// Fetch the things we need for distortion cost computation.
	int maxDistortion = StaticData::Instance().GetMaxDistortion();
	const InputType *itype = StaticData::Instance().GetInput();
	WordsRange transOptRange = translations[0]->GetSourceWordsRange();

	m_hypothesis_maxpos = 0;
	OrderedHypothesisSet::const_iterator hypoIter = hypotheses.begin();
	for (; m_hypothesis_maxpos < m_kbest && hypoIter != hypotheses.end(); ++hypoIter)
	{
		// If the combination of this hypothesis and our translation
		// options violates the distortion limit, discard the hypothesis,
		// otherwise store it and increment m_hypothesis_maxpos.		
		Hypothesis *current = *hypoIter;

		if (maxDistortion == -1)
		{
			m_kbest_hypotheses[m_hypothesis_maxpos++] = current;
		}
		else
		{
		  int distortionDistance = itype->ComputeDistortionDistance(current->GetCurrSourceWordsRange()
																																, transOptRange);
		  if (distortionDistance <= maxDistortion)
			{
				m_kbest_hypotheses[m_hypothesis_maxpos++] = current;
			}
		}
	}
	
	// Maybe the list has shrunk.
	m_kbest_hypotheses.resize(m_hypothesis_maxpos);
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
	m_hypotheses = OrderedHypothesisSet();
	m_edges = BackwardsEdgeSet();
	m_queue = HypothesisQueue();
}

BitmapContainer::~BitmapContainer()
{
	// As we have created the square position objects we clean up now.
	for (size_t i=0; i<m_queue.size(); i++)
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
	m_queue.push(item);
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
	BackwardsEdgeSet::iterator iter;
	for (iter = m_edges.begin(); iter != m_edges.end(); ++iter)
	{
		BackwardsEdge *edge = *iter;
		edge->Initialize();
	}

	for (size_t i=0; i<m_kbest; i++)
	{
		if (m_queue.empty())
		{
			return;
		}

		// Get the currently best hypothesis from the queue.
		HypothesisQueueItem *item = Dequeue();

		// If the priority queue is exhausted, we are done and exit.
		if (item == NULL)
		{
			return;
		}

		// Add best hypothesis to hypothesis stack.
		bool added = m_stack.AddPrune(item->GetHypothesis());	
		if (added)
		{
			m_kbest++;
		}

		// Create new hypotheses for the two successors of the hypothesis just added.
		int hypothesis_pos = item->GetHypothesisPos();
		int translation_pos = item->GetTranslationPos();
		item->GetBackwardsEdge()->PushSuccessors(hypothesis_pos, translation_pos);
	}
}
