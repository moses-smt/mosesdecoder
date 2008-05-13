#pragma once

#include <queue>
#include <set>
#include "Hypothesis.h"
#include "TypeDef.h"
#include "WordsBitmap.h"

class BitmapContainer;
class BackwardsEdge;
class Hypothesis;

/** Order relation for hypothesis scores.  Taken from Eva Hasler's branch. */
class HypothesisScoreOrderer
{
public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		// Get score so far
		float scoreA = hypoA->GetTotalScore();
		float scoreB = hypoB->GetTotalScore();
		return (scoreA >= scoreB);
	}
};

typedef std::set< Hypothesis*, HypothesisScoreOrderer > OrderedHypothesisSet;
typedef std::set< BackwardsEdge* > BackwardsEdgeSet;
typedef std::pair< float, std::pair< int, int > > SquarePosition;

// Allows to compare two square positions (coordinates) by the corresponding scores.
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

// Encodes an edge pointing to a BitmapContainer and an associated priority queue
// that contains the square scores and the corresponding square coordinates.
class BackwardsEdge
{
	private:
		typedef std::priority_queue< SquarePosition*, std::vector< SquarePosition* >, SquarePositionOrderer> _PQType;
		const BitmapContainer &m_prevBitmapContainer;
		_PQType m_queue;
		static SquarePosition *m_invalid;
		TranslationOptionList m_translations;
		
		BackwardsEdge();

	public:
		const SquarePosition InvalidSquarePosition();

		BackwardsEdge(const BitmapContainer &prevBitmapContainer, TranslationOptionList translations);
		~BackwardsEdge();

		const BitmapContainer &GetBitmapContainer() const;
		int GetDistortionPenalty();
		void Enqueue(int x, int y, float score);
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

		// We always require a corresponding bitmap to be supplied.
		BitmapContainer();

	public:
		BitmapContainer(const WordsBitmap &bitmap);
		
		// The destructor will also delete all the edges that are
		// connected to this BitmapContainer.
		~BitmapContainer();
		
		const WordsBitmap &GetWordsBitmap();
		const OrderedHypothesisSet &GetHypotheses();
		const BackwardsEdgeSet &GetBackwardsEdges();
		
		// We will add GetKBest() here.

		void AddHypothesis(Hypothesis *hypothesis);
		void AddBackwardsEdge(BackwardsEdge *edge);
		void PruneHypotheses();
};