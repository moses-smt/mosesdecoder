#pragma once

#include "WordsBitmap.h"
#include "Hypothesis.h"
#include "HypothesisStack.h"
#include <queue>

class BitmapContainer;
class BackwardsEdge;

typedef std::set< Hypothesis*, HypothesisScoreOrderer > OrderedHypothesisSet;
typedef std::set< BackwardsEdge* > BackwardsEdgeSet;
typedef std::pair< float, std::pair< int, int > > SquarePosition;

class SquarePositionOrderer
{
	public:
		bool operator()(const SquarePosition* cellA, const SquarePosition* cellB) const
		{
			float scoreA = cellA->first;
			float scoreB = cellB->first;
			return (scoreA >= scoreB);
		}
};

class BackwardsEdge
{
	private:
		typedef std::priority_queue< SquarePosition*, std::vector< SquarePosition* >, SquarePositionOrderer> _PQType;
		BitmapContainer *m_prev;
		_PQType m_queue;
		
		BackwardsEdge();

	public:
		BackwardsEdge(BitmapContainer *prev);
		~BackwardsEdge();

		BitmapContainer *GetBitmapContainer();
		void Enqueue(SquarePosition *chunk);
		SquarePosition *Dequeue();
};

class BitmapContainer
{
	private:
		WordsBitmap m_bitmap;
		OrderedHypothesisSet m_hypotheses;
		BackwardsEdgeSet m_edges;

		// We always require a corresponding bitmap to be supplied.
		BitmapContainer();

	public:
		BitmapContainer(const WordsBitmap &bitmap);
		~BitmapContainer();
		
		const WordsBitmap &GetWordsBitmap();
		const OrderedHypothesisSet &GetHypotheses();
		const BackwardsEdgeSet &GetBackwardsEdges();

		void AddHypothesis(Hypothesis *hypothesis);
		void AddBackwardsEdge(BackwardsEdge *edge);
		void PruneHypotheses();
};