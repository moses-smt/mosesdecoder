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

#pragma once

#include <queue>
#include <set>
#include "Hypothesis.h"
#include "SquareMatrix.h"
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


/** Order relation for hypothesis scores.  Taken from Eva Hasler's branch. */
class HypothesisScoreOrderer
{
	public:
		bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
		{
			float scoreA = hypoA->GetTotalScore();
			float scoreB = hypoB->GetTotalScore();
			
			if (scoreA > scoreB)
			{
				return true;
			}
			else if (scoreA < scoreB)
			{
				return false;
			}
			else
			{
				return hypoA < hypoB;
			}
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
			return (scoreA > scoreB);
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
		std::vector< bool > m_seenPosition;
		bool m_initialized;
		const SquareMatrix &m_futurescore;
		size_t m_kbest;
		size_t m_xmax, m_ymax;

		const TranslationOptionList &m_kbest_translations;
		std::vector< Hypothesis* > m_kbest_hypotheses;
		
		BackwardsEdge();

		Hypothesis *CreateHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt);
		void Initialize();

	public:
		const SquarePosition InvalidSquarePosition();

		BackwardsEdge(const BitmapContainer &prevBitmapContainer
					  , const TranslationOptionList &translations
					  , const SquareMatrix &futureScore
					  , const size_t KBestCubePruning);
		~BackwardsEdge();

		bool GetInitialized();
		const BitmapContainer &GetBitmapContainer() const;
		int GetDistortionPenalty();
		bool Empty();
		size_t Size();
		void PushSuccessors(int x, int y);
		void Enqueue(int x, int y, Hypothesis *hypothesis);
		SquarePosition Dequeue(bool keepValue=false);
		bool SeenPosition(int x, int y);
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
		HypothesisStack &m_stack;
		size_t m_kbest;

		// We always require a corresponding bitmap to be supplied.
		BitmapContainer();
		

	public:
		BitmapContainer(const WordsBitmap &bitmap
						, HypothesisStack &stack
						, const size_t KBestCubePruning);
		
		// The destructor will also delete all the edges that are
		// connected to this BitmapContainer.
		~BitmapContainer();
		
		const WordsBitmap &GetWordsBitmap();
		const OrderedHypothesisSet &GetHypotheses() const;
		const BackwardsEdgeSet &GetBackwardsEdges();
		
		void FindKBestHypotheses();

		void AddHypothesis(Hypothesis *hypothesis);
		void AddBackwardsEdge(BackwardsEdge *edge);
};


