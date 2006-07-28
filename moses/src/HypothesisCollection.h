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
	static size_t s_ngramMaxOrder[NUM_FACTORS];

public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
    // this function defines less-than relation on hypotheses
    // the particular order is not important for us, we need just to figure out
    // which hypothesis are equal based on:
    //   the last n-1 target words are the same
    //   and the covers (source words translated) are the same
	{
        // Are the last (n-1) words the same on the target side (n for n-gram LM)?
		int ret = hypoA->NGramCompare(*hypoB);
//		int ret = hypoA->FastNGramCompare(*hypoB, m_NGramMaxOrder - 1);
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
	static inline void SetMaxNGramOrder(FactorType factorType, size_t ngramMaxOrder)
	{
		assert((size_t)factorType < NUM_FACTORS);
		if (s_ngramMaxOrder[factorType] < ngramMaxOrder)
			s_ngramMaxOrder[factorType] = ngramMaxOrder;
	}
};

class HypothesisCollection 
{
public:
	typedef std::set< Hypothesis*, CompareHypothesisCollection >::iterator iterator;
	typedef std::set< Hypothesis*, CompareHypothesisCollection >::const_iterator const_iterator;
	friend std::ostream& operator<<(std::ostream&, const HypothesisCollection&);

protected:
	float m_bestScore;
    float m_worstScore;
    float m_beamThreshold;
	size_t m_maxHypoStackSize;
  std::set< Hypothesis*, CompareHypothesisCollection > m_hypos;


	void Add(Hypothesis *hypothesis);
		// if returns false, hypothesis not used
		// caller must take care to delete unused hypo to avoid leak
		// used by Add(Hypothesis *hypothesis, float beamThreshold);
	void RemoveAll();

	inline void Detach(const HypothesisCollection::iterator &iter)
	{
		m_hypos.erase(iter);
	}
	inline void Remove(const HypothesisCollection::iterator &iter)
	{
		delete *iter;
		Detach(iter);
	}
	inline void AddNoPrune(Hypothesis *hypothesis)
	{
		if (!m_hypos.insert(hypothesis).second) {
    }
	}

public:
	const_iterator begin() const { return m_hypos.begin(); }
	const_iterator end() const { return m_hypos.end(); }
	size_t size() const { return m_hypos.size(); }

	inline HypothesisCollection()
	{
		m_bestScore = -std::numeric_limits<float>::infinity();
		m_worstScore = -std::numeric_limits<float>::infinity();
	}

	// this function will recombine hypotheses silently!  There is no record
	// (could affect n-best list generation...TODO)
	bool AddPrune(Hypothesis *hypothesis);
      // AddPrune adds the hypo, but only if within thresholds (beamThr+stackSize)

	inline ~HypothesisCollection()
	{
		RemoveAll();
	}
	inline void SetMaxHypoStackSize(size_t maxHypoStackSize)
	{
		m_maxHypoStackSize = maxHypoStackSize;
	}
	inline void SetBeamThreshold(float beamThreshold)
	{
		m_beamThreshold = beamThreshold;
	}
	inline float GetBestScore() const
	{
		return m_bestScore;
	}
	
	//void Prune();
	void PruneToSize(size_t newSize);

	const Hypothesis *GetBestHypothesis() const;
	std::vector<const Hypothesis*> GetSortedList() const;
	void InitializeArcs();
	
	TO_STRING;
	
};
