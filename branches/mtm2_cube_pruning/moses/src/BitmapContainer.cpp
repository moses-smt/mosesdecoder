#include <algorithm>
#include <limits>
#include <utility>
#include "BitmapContainer.h"
#include "HypothesisStack.h"

SquarePosition *BackwardsEdge::m_invalid = NULL;

BackwardsEdge::BackwardsEdge(const BitmapContainer &prevBitmapContainer, const TranslationOptionList &translations)
  : m_prevBitmapContainer(prevBitmapContainer)
	, m_queue()
{
	// QUESTION: can we expect our translation parameter to be NOT EMPTY? We should?!

	// We will copy the k best translation options from the translations
	// parameter and keep them in a std::vector to allow fast access.

	// IMPORTANT! We ASSUME that the given TranslationOptionList object is already sorted!
	// Hence it is the duty of the TranslationOptionCollection to keep this list sorted...

	// We will copy either k options or less if the translations parameter does not include k.
	//
	// TODO: replace 0 by k-best constant!
	//
	size_t kBest = 0; // Call this? staticData.GetKBestCubePruning()
	size_t k = std::min(kBest, translations.size()); // WHERE do we get the k-best parameter?
	
	// We reserve exactly as much space as we need to avoid resizing.
	m_kbest_translations.reserve(k);
	std::copy(translations.begin(), translations.begin() + k, m_kbest_translations.begin());

	// We should also do this for the hypotheses that are attached to the BitmapContainer
	// which this backwards edge points to :)  Same story: compute k, copy k hypotheses
	// from the OrderedHypothesisSet in the BitmapContainer and voila...
	const OrderedHypothesisSet &hypotheses = m_prevBitmapContainer.GetHypotheses();
	k = std::min(kBest, hypotheses.size());
	
	OrderedHypothesisSet::const_iterator hypoEnd = hypotheses.begin();
	for (size_t i=0; i<k; i++) {
		hypoEnd++;
	}
	m_kbest_hypotheses.reserve(k);
	std::copy(hypotheses.begin(), hypoEnd, m_kbest_hypotheses.begin());
	
	// The BackwardsEdge now has ALL data it needs to perform cube pruning :)
	
	// Expand first (0, 0) hypothesis and put it into the queue.
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
		delete ret;
		m_queue.pop();
	}
}

const BitmapContainer&
BackwardsEdge::GetBitmapContainer() const
{
	return m_prevBitmapContainer;
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
}

SquarePosition
BackwardsEdge::Dequeue(bool keepValue)
{
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

BitmapContainer::BitmapContainer(const WordsBitmap &bitmap, const HypothesisStack &stack)
  : m_bitmap(bitmap)
	, m_stack(stack)
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