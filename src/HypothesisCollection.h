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

#ifdef __GNUG__
#include <ext/hash_set>
#endif

class HypothesisRecombinationOrderer
{
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
};

struct HypothesisRecombinationComparer
{
	//! returns true if hypoA can be recombined with hypoB
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		if (hypoA->NGramCompare(*hypoB) != 0) return false;
		return (hypoA->GetWordsBitmap().Compare(hypoB->GetWordsBitmap()) == 0);
	}
};

struct HypothesisRecombinationHasher
{
  size_t operator()(const Hypothesis* hypo) const {
    return hypo->hash();
  }
};

class HypothesisCollection 
{
private:
#if 0
//#ifdef __GNUG__
	typedef __gnu_cxx::hash_set< Hypothesis*, HypothesisRecombinationHasher, HypothesisRecombinationComparer > _HCType;
#else
	typedef std::set< Hypothesis*, HypothesisRecombinationOrderer > _HCType;
#endif
public:
	typedef _HCType::iterator iterator;
	typedef _HCType::const_iterator const_iterator;
	friend std::ostream& operator<<(std::ostream&, const HypothesisCollection&);

protected:
	float m_bestScore;
    float m_worstScore;
    float m_beamThreshold;
	size_t m_maxHypoStackSize;
  _HCType m_hypos;


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
		ObjectPool<Hypothesis> &pool = Hypothesis::GetObjectPool();
		pool.freeObject(*iter);
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
	void AddPrune(Hypothesis *hypothesis);
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
