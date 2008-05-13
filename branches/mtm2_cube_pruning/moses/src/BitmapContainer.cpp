#include "BitmapContainer.h"

BackwardsEdge::BackwardsEdge(BitmapContainer *prev)
  : m_prev(prev)
{
	m_queue = _PQType();
}

BackwardsEdge::~BackwardsEdge()
{
	for (size_t q_iter = 0; q_iter < m_queue.size(); q_iter++)
	{
		m_queue.pop();
	}
}

BitmapContainer*
BackwardsEdge::GetBitmapContainer()
{
	return m_prev;
}

void
BackwardsEdge::Enqueue(SquarePosition *chunk)
{
	m_queue.push(chunk);
}


SquarePosition*
BackwardsEdge::Dequeue()
{
	if (!m_queue.empty()) {
		SquarePosition* ret = m_queue.top();
		m_queue.pop();
		return ret;
	}

	return NULL;
}

BitmapContainer::BitmapContainer(const WordsBitmap &bitmap)
  : m_bitmap(bitmap)
{
	m_hypotheses = OrderedHypothesisSet();
	m_edges = BackwardsEdgeSet();
}

BitmapContainer::~BitmapContainer()
{
	BackwardsEdgeSet::iterator e_iter;
	for (e_iter = m_edges.begin(); e_iter != m_edges.end(); ++e_iter)
	{
		delete *e_iter;
	}

	m_hypotheses.clear();
	m_edges.clear();
}
		
const WordsBitmap&
BitmapContainer::GetWordsBitmap()
{
	return m_bitmap;
}

const OrderedHypothesisSet&
BitmapContainer::GetHypotheses()
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
	m_hypotheses.insert(hypothesis);
}

void
BitmapContainer::AddBackwardsEdge(BackwardsEdge *edge)
{
	m_edges.insert(edge);
}

void
BitmapContainer::PruneHypotheses()
{
	// NOT IMPLEMENTED YET.
}