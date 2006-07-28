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

#include <cstring> //memcpy()
#include <iostream>
#include <list>
#include "TypeDef.h"
#include "Phrase.h"
#include "ScoreComponentCollection.h"
#include "DecodeStep.h"

class Hypothesis;
class Arc;
class GenerationDictionary;


class LatticeEdge
{
	friend std::ostream& operator<<(std::ostream& out, const LatticeEdge& edge);
	
protected:
	// scores
	float						m_score[NUM_SCORES];

	const Hypothesis* m_prevHypo;
	Phrase					m_targetPhrase; //target phrase being created at the current decoding step

#ifdef N_BEST
	ScoreComponentCollection2 m_scoreBreakdown;
#endif

public:
	LatticeEdge(const LatticeEdge &copy); // not implemented
	LatticeEdge(const float 												score[]
						, const Phrase 												&phrase
						, const Hypothesis 										*prevHypo
						,	const ScoreComponentCollection2	&scoreBreakdown)
		:m_prevHypo(prevHypo)
		,m_targetPhrase(phrase)
#ifdef N_BEST
		,m_scoreBreakdown(scoreBreakdown)
#endif
	{
		SetScore(score);
	}

	LatticeEdge(FactorDirection direction)
	:m_prevHypo(NULL)
	,m_targetPhrase(direction)
	{
	}
	LatticeEdge(FactorDirection direction, const Hypothesis *prevHypo);

	virtual ~LatticeEdge();

	inline const Phrase &GetTargetPhrase() const
	{
		return m_targetPhrase;
	}
	inline void SetFactor(size_t pos, FactorType factorType, const Factor *factor)
	{ // pos starts from current phrase, not from beginning of 1st phrase
		m_targetPhrase.SetFactor(pos, factorType, factor);
	}
	/***
	 * score should be of length NUM_SCORES
	 */
	inline void SetScore(const float score[])
	{
		std::memcpy(m_score, score, NUM_SCORES * sizeof(float));
	}
	void ResetScore();

	inline const float *GetScore() const
	{
		return m_score;
	}
	inline float GetScore(ScoreType::ScoreType scoreType) const
	{
		return m_score[scoreType];
	}

	inline const Hypothesis *GetPrevHypo() const
	{
		return m_prevHypo;
	}

#ifdef N_BEST
	virtual const std::vector<Arc*>* GetArcList() const = 0;
	const ScoreComponentCollection2& GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

#if 0	
	void ResizeComponentScore(const LMList &allLM, const std::list < DecodeStep > &decodeStepList);
#endif
#endif

	TO_STRING;

};

inline std::ostream& operator<<(std::ostream& out, const LatticeEdge& edge)
{
	out << edge.GetTargetPhrase();
	return out;
}

