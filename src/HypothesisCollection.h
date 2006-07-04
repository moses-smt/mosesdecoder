// $Id$

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

#include <limits>
#include <set>
#include "Hypothesis.h"

class CompareHypothesisCollection
{
protected:
	// static
	static size_t m_NGramMaxOrder;

public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		int ret = hypoA->NGramCompare(*hypoB, m_NGramMaxOrder - 1);
		if (ret != 0)
		{
			return (ret < 0);
		}

		// same last n-grams. compare source words translated
		const WordsBitmap &bitmapA		= hypoA->GetWordsBitmap()
			, &bitmapB	= hypoB->GetWordsBitmap();
		ret = bitmapA.Compare(bitmapB);

		return (ret < 0);
	}

	// static 
	static inline void SetMaxNGramOrder(size_t nGramMaxOrder)
	{
		m_NGramMaxOrder = nGramMaxOrder;
	}
	static inline size_t GetMaxNGramOrder()
	{
		return m_NGramMaxOrder;
	}

};

class HypothesisCollection : public std::set< Hypothesis*, CompareHypothesisCollection >
{
	friend std::ostream& operator<<(std::ostream&, const HypothesisCollection&);

protected:
	float m_bestScore, m_beamThreshold;

//	std::list<Arc> m_arc;
	void Add(Hypothesis *hypothesis);
		// used by Add(Hypothesis *hypothesis, float beamThreshold);
	void RemoveAll();

public:
	inline HypothesisCollection()
	{
		m_bestScore = -std::numeric_limits<float>::infinity();
	}

	inline void AddNoPrune(Hypothesis *hypothesis)
	{
		//push_back(hypothesis);
		insert(hypothesis);
	}
	bool Add(Hypothesis *hypothesis, float beamThreshold);
	inline void Detach(const HypothesisCollection::iterator &iter)
	{
		erase(iter);
	}
	inline void Remove(const HypothesisCollection::iterator &iter)
	{
		delete *iter;
		Detach(iter);
	}

	inline ~HypothesisCollection()
	{
		RemoveAll();
	}
	inline void SetBeamThreshold(float beamThreshold)
	{
		m_beamThreshold = beamThreshold;
	}

	//void Prune();
	void PruneToSize(size_t newSize);

	const Hypothesis *GetBestHypothesis() const;
	std::list<const Hypothesis*> GetSortedList() const;
	void InitializeArcs();
};
