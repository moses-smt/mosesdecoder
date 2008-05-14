#pragma once

#include <queue>
#include <set>
#include "Hypothesis.h"
#include "TranslationOption.h"
#include "TypeDef.h"
#include "WordsBitmap.h"

class BitmapContainer;
class BackwardsEdge;
class Hypothesis;
class HypothesisScoreOrderer;
class HypothesisStack;


typedef std::set< Hypothesis*, HypothesisScoreOrderer > OrderedHypothesisSet;
typedef std::set< BackwardsEdge* > BackwardsEdgeSet;
typedef std::pair< Hypothesis*, std::pair< int, int > > SquarePosition;


/** Order relation for TranslationOption scores. */
class TranslationOptionOrderer
{
	public:
		bool operator()(const TranslationOption* optionA, const TranslationOption* optionB) const
		{
			float scoreA = optionA->GetFutureScore();
			float scoreB = optionB->GetFutureScore();
			return (scoreA >= scoreB);
		}
};

/** Order relation for hypothesis scores.  Taken from Eva Hasler's branch. */
class HypothesisScoreOrderer
{
	public:
		bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
		{
			float scoreA = hypoA->GetTotalScore();
			float scoreB = hypoB->GetTotalScore();
			return (scoreA >= scoreB);
		}
};

// Allows to compare two square positions (coordinates) by the corresponding scores.
class SquarePositionOrderer
{
	public:
		bool operator()(const SquarePosition* cellA, const SquarePosition* cellB) const
		{
			float scoreA = cellA->first->GetTotalScore();
			float scoreB = cellB->first->GetTotalScore();
			return (scoreA >= scoreB);
		}
};

// Encodes an edge pointing to a BitmapContainer and an associated priority queue
// that contains the square scores and the corresponding square coordinates.
class BackwardsEdge
{
	private:
		typedef std::priority_queue< SquarePosition*, std::vector< SquarePosition* >, SquarePositionOrderer> _PQType;
		const BitmapContainer &m_prevBitmapContainer;
		_PQType m_queue;
		static SquarePosition *m_invalid;
		bool m_initialized;

		std::vector< TranslationOption* > m_kbest_translations;
		std::vector< Hypothesis* > m_kbest_hypotheses;
		
		BackwardsEdge();

	public:
		const SquarePosition InvalidSquarePosition();

		BackwardsEdge(const BitmapContainer &prevBitmapContainer, const TranslationOptionList &translations);
		~BackwardsEdge();

		bool GetInitialized();
		void SetInitialized(bool initialized);
		void Initialize(Hypothesis *hypothesis);
		const BitmapContainer &GetBitmapContainer() const;
		int GetDistortionPenalty();
		bool Empty();
		size_t Size();
		void Enqueue(int x, int y, Hypothesis *hypothesis);
		SquarePosition Dequeue(bool keepValue=false);
};

// A BitmapContainer encodes an ordered set of hypotheses and a set of edges
// pointing to the "generating" BitmapContainers.  This data logically belongs
// to the bitmap coverage which is stored in m_bitmap.
class BitmapContainer
{
	private:
		WordsBitmap m_bitmap;
		OrderedHypothesisSet m_hypotheses;
		BackwardsEdgeSet m_edges;
		const HypothesisStack &m_stack;

		// We always require a corresponding bitmap to be supplied.
		BitmapContainer();

	public:
		BitmapContainer(const WordsBitmap &bitmap, const HypothesisStack &stack);
		
		// The destructor will also delete all the edges that are
		// connected to this BitmapContainer.
		~BitmapContainer();
		
		const WordsBitmap &GetWordsBitmap();
		const OrderedHypothesisSet &GetHypotheses() const;
		const BackwardsEdgeSet &GetBackwardsEdges();
		
		// We will add GetKBest() here.

		void AddHypothesis(Hypothesis *hypothesis);
		void AddBackwardsEdge(BackwardsEdge *edge);
		void PruneHypotheses();
};