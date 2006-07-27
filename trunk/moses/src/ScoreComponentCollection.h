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

#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <assert.h>
#include "ScoreComponent.h"

class Dictionary;

class ScoreComponentCollection
{
	friend std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &scoreComponentColl);
private:
	std::map<const Dictionary*, ScoreComponent> m_scores;
public:
	typedef std::map<const Dictionary*, ScoreComponent>::iterator iterator;
	typedef std::map<const Dictionary*, ScoreComponent>::const_iterator const_iterator;
	iterator begin() { return m_scores.begin(); }
	iterator end() { return m_scores.end(); }
	const_iterator begin() const { return m_scores.begin(); }
	const_iterator end() const { return m_scores.end(); }
	
	ScoreComponent &GetScoreComponent(const Dictionary *dictionary)
	{
		iterator iter = m_scores.find(dictionary);
		assert(iter != m_scores.end());
		return iter->second;
	}

	const ScoreComponent &GetScoreComponent(const Dictionary *dictionary) const
	{
		return const_cast<ScoreComponentCollection*>(this)->GetScoreComponent(dictionary);
	}

	void Remove(const Dictionary *dictionary)
	{
		m_scores.erase(dictionary);
	}
	
	ScoreComponent &Add(const ScoreComponent &scoreComponent)
	{
		const Dictionary *dictionary = scoreComponent.GetDictionary();
		assert( dictionary != NULL && m_scores.find(dictionary) == m_scores.end());
		return m_scores[dictionary] = scoreComponent;
	}

	ScoreComponent &Add(const Dictionary *dictionary)
	{
		return Add(ScoreComponent(dictionary));
	}

	void Combine(const ScoreComponentCollection &otherComponentCollection);
	std::vector<const ScoreComponent*> SortForNBestOutput() const;
};

inline std::ostream& operator<<(std::ostream &out, const ScoreComponentCollection &scoreComponentColl)
{
	ScoreComponentCollection::const_iterator iter;
	for (iter = scoreComponentColl.m_scores.begin() ; iter != scoreComponentColl.m_scores.end() ; ++iter)
	{
		const ScoreComponent &scoreComponent = iter->second;
		out << "[" << scoreComponent << "] ";
	}
	return out;
}

///////////////////////////
// UNUSED AT PRESENT (8/27, 8am), but this will be used to replace ScoreComponentCollection soon

#include <numeric>
#include "ScoreProducer.h"
#include "StaticData.h"

class ScoreComponentCollection2 {

private:
	std::vector<float> m_scores;
	const ScoreIndexManager& m_sim;

public:
	ScoreComponentCollection2()
	: m_scores(StaticData::Instance()->GetTotalScoreComponents(), 0.0f)
	, m_sim(StaticData::Instance()->GetScoreIndexManager())
	{}

	ScoreComponentCollection2(const ScoreComponentCollection2& rhs)
	: m_scores(rhs.m_scores)
	, m_sim(rhs.m_sim)
	{}

	void PlusEquals(const ScoreComponentCollection2& rhs)
	{
		assert(m_scores.size() == rhs.m_scores.size());
		const size_t l = m_scores.size();
		for (size_t i=0; i<l; i++) { m_scores[i] += rhs.m_scores[i]; }  
	}

	//! Add scores from a single ScoreProducer only
	//! The length of scores must be equal to the number of score components
	//! produced by sp
	void PlusEquals(const ScoreProducer* sp, const std::vector<float>& scores)
	{
		assert(scores.size() != sp->GetNumScoreComponents());
		size_t i = m_sim.GetBeginIndex(sp->GetScoreBookkeepingID());
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
		const size_t i = m_sim.GetBeginIndex(sp->GetScoreBookkeepingID());
		m_scores[i] += score;
	}

	float InnerProduct(const std::vector<float>& rhs) const
	{
		return std::inner_product(m_scores.begin(), m_scores.end(), rhs.begin(), 0.0f);
	}

	float GetTotalScoreForScoreProducer(const ScoreProducer* sp) const
	{
		size_t id = sp->GetScoreBookkeepingID();
		const size_t begin = m_sim.GetBeginIndex(id);
		const size_t end = m_sim.GetEndIndex(id);
		float total = 0.0f;
		for (size_t i = begin; i < end; i++) {
			total += m_scores[i];
		}
		return total;
	}

	//! return a vector of all the scores associated with a certain ScoreProducer
	std::vector<float> GetScoresForProducer(const ScoreProducer* sp) const
	{
		size_t id = sp->GetScoreBookkeepingID();
		const size_t begin = m_sim.GetBeginIndex(id);
		const size_t end = m_sim.GetEndIndex(id);
		std::vector<float> res(end-begin);
		for (size_t i = begin; i < end; i++) {
			res.push_back(m_scores[i]);
		}
		return res;  
	}

	//! if a ScoreProducer produces a single score (for example, a language model score)
	//! this will return it.  If not, this method will throw
	float GetScoreForProducer(const ScoreProducer* sp) const
	{
		size_t id = sp->GetScoreBookkeepingID();
		const size_t begin = m_sim.GetBeginIndex(id);
		const size_t end = m_sim.GetEndIndex(id);
		assert(end-begin == 1);
		return m_scores[begin];
	}

};

