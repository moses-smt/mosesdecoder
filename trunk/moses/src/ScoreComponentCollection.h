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

#include <numeric>
#include "ScoreProducer.h"
#include "ScoreIndexManager.h"

class ScoreComponentCollection2 {
  friend std::ostream& operator<<(std::ostream& os, const ScoreComponentCollection2& rhs);
private:
	std::vector<float> m_scores;
	const ScoreIndexManager* m_sim;

public:
	ScoreComponentCollection2();

	ScoreComponentCollection2(const ScoreComponentCollection2& rhs)
	: m_scores(rhs.m_scores)
	, m_sim(rhs.m_sim)
	{}

	void ZeroAll()
	{
		for (std::vector<float>::iterator i=m_scores.begin(); i!=m_scores.end(); ++i)
			*i = 0.0f;
	}

	void PlusEquals(const ScoreComponentCollection2& rhs)
	{
		assert(m_scores.size() >= rhs.m_scores.size());
		const size_t l = rhs.m_scores.size();
		for (size_t i=0; i<l; i++) { m_scores[i] += rhs.m_scores[i]; }  
	}

	void MinusEquals(const ScoreComponentCollection2& rhs)
	{
		assert(m_scores.size() >= rhs.m_scores.size());
		const size_t l = rhs.m_scores.size();
		for (size_t i=0; i<l; i++) { m_scores[i] -= rhs.m_scores[i]; }  
	}

	//! Add scores from a single ScoreProducer only
	//! The length of scores must be equal to the number of score components
	//! produced by sp
	void PlusEquals(const ScoreProducer* sp, const std::vector<float>& scores)
	{
		assert(scores.size() == sp->GetNumScoreComponents());
		size_t i = m_sim->GetBeginIndex(sp->GetScoreBookkeepingID());
		for (std::vector<float>::const_iterator vi = scores.begin();
		     vi != scores.end(); ++vi)
		{
			m_scores[i++] += *vi;
		}  
	}

	//! Special version PlusEquals(ScoreProducer, vector<float>)
	//! to add the score from a single ScoreProducer that produces
	//! a single value
	void PlusEquals(const ScoreProducer* sp, float score)
	{
		assert(1 == sp->GetNumScoreComponents());
		const size_t i = m_sim->GetBeginIndex(sp->GetScoreBookkeepingID());
		m_scores[i] += score;
	}

	//! Special version PlusEquals(ScoreProducer, vector<float>)
	//! to add the score from a single ScoreProducer that produces
	//! a single value
	void Assign(const ScoreProducer* sp, float score)
	{
		assert(1 == sp->GetNumScoreComponents());
		const size_t i = m_sim->GetBeginIndex(sp->GetScoreBookkeepingID());
		m_scores[i] = score;
	}

	float InnerProduct(const std::vector<float>& rhs) const
	{
		return std::inner_product(m_scores.begin(), m_scores.end(), rhs.begin(), 0.0f);
	}

	float PartialInnerProduct(const ScoreProducer* sp, const std::vector<float>& rhs) const
	{
		std::vector<float> lhs = GetScoresForProducer(sp);
		assert(lhs.size() == rhs.size());
		return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0f);
	}

	//! return a vector of all the scores associated with a certain ScoreProducer
	std::vector<float> GetScoresForProducer(const ScoreProducer* sp) const
	{
		size_t id = sp->GetScoreBookkeepingID();
		const size_t begin = m_sim->GetBeginIndex(id);
		const size_t end = m_sim->GetEndIndex(id);
		std::vector<float> res(end-begin);
		size_t j = 0;
		for (size_t i = begin; i < end; i++) {
			res[j++] = m_scores[i];
		}
		return res;  
	}

	//! if a ScoreProducer produces a single score (for example, a language model score)
	//! this will return it.  If not, this method will throw
	float GetScoreForProducer(const ScoreProducer* sp) const
	{
		size_t id = sp->GetScoreBookkeepingID();
		const size_t begin = m_sim->GetBeginIndex(id);
		const size_t end = m_sim->GetEndIndex(id);
		assert(end-begin == 1);
		return m_scores[begin];
	}

};

inline std::ostream& operator<<(std::ostream& os, const ScoreComponentCollection2& rhs)
{
  os << "<<" << rhs.m_scores[0];
  for (size_t i=1; i<rhs.m_scores.size(); i++)
    os << ", " << rhs.m_scores[i];
  return os << ">>";
}

