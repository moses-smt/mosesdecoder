#include <algorithm>
#include <limits>
#include <utility>
#include "BitmapContainer.h"
#include "HypothesisStack.h"

SquarePosition *BackwardsEdge::m_invalid = NULL;

BackwardsEdge::BackwardsEdge(const BitmapContainer &prevBitmapContainer, const TranslationOptionList &translations)
  : m_prevBitmapContainer(prevBitmapContainer)
	, m_queue()
	, m_initialized(false)
{
	// We will copy the k best translation options from the translations
	// parameter and keep them in a std::vector to allow fast access.
	// We will copy at most k translation options to the vector.
	const StaticData &staticData = StaticData::Instance();
	size_t kBest = staticData.GetKBestCubePruning();
	size_t k = std::min(kBest, translations.size());
	
	TranslationOptionList::iterator optionsEnd = translations.begin();
	for (size_t i=0; i<k; i++) {
		optionsEnd++;
	}

	// We reserve exactly as much space as we need to avoid resizing.
	m_kbest_translations.reserve(k);
	std::copy(translations.begin(), translations.begin() + k, m_kbest_translations.begin());

	// We should also do this for the hypotheses that are attached to the BitmapContainer
	// which this backwards edge points to :)  Same story: compute k, copy k hypotheses
	// from the OrderedHypothesisSet in the BitmapContainer et voila...
	const OrderedHypothesisSet &hypotheses = m_prevBitmapContainer.GetHypotheses();
	k = std::min(kBest, hypotheses.size());
	
	OrderedHypothesisSet::const_iterator hypoEnd = hypotheses.begin();
	for (size_t i=0; i<k; i++) {
		hypoEnd++;
	}

	// We reserve exactly as much space as we need to avoid resizing.
	m_kbest_hypotheses.reserve(k);
	std::copy(hypotheses.begin(), hypoEnd, m_kbest_hypotheses.begin());
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

bool
BackwardsEdge::GetInitialized()
{
	return m_initialized;
}

void
BackwardsEdge::SetInitialized(bool initialized)
{
	m_initialized = initialized;
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