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
#include <list>
#include "TypeDef.h"
#include "Phrase.h"
#include "ScoreComponent.h"
#include "ScoreComponentCollection.h"
#include "ScoreColl.h"
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

	const Hypothesis *m_prevHypo;
	Phrase					m_phrase;

#ifdef N_BEST
	ScoreComponentCollection	m_transScoreComponent;
	ScoreColl						m_generationScoreComponent
											,m_lmScoreComponent;
#endif

public:
	LatticeEdge(const LatticeEdge &copy); // not implemented
	LatticeEdge(const float 												score[NUM_SCORES]
						, const ScoreComponentCollection 	&transScoreComponent
						, const ScoreColl					 						&lmScoreComponent
						, const ScoreColl											&generationScoreComponent
						, const Phrase 												&phrase
						, const Hypothesis 										*prevHypo)
		:m_prevHypo(prevHypo)
		,m_phrase(phrase)
#ifdef N_BEST
		,m_transScoreComponent(transScoreComponent)
		,m_generationScoreComponent(generationScoreComponent)
		,m_lmScoreComponent		(lmScoreComponent)
#endif
	{
		SetScore(score);
	}
	LatticeEdge(FactorDirection direction, const Hypothesis *prevHypo)
		:m_prevHypo(prevHypo)
		,m_phrase(direction)
	{}
	virtual ~LatticeEdge()
	{
	}

	inline const Phrase &GetPhrase() const
	{
		return m_phrase;
	}
	inline void SetFactor(size_t pos, FactorType factorType, const Factor *factor)
	{ // pos starts from current phrase, not from beginning of 1st phrase
		m_phrase.SetFactor(pos, factorType, factor);
	}
	inline void SetScore(const float score[NUM_SCORES])
	{
		for (size_t currScore = 0 ; currScore < NUM_SCORES ; currScore++)
		{
			m_score[currScore] = score[currScore];
		}
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
	virtual const std::list<Arc*> &GetArcList() const = 0;

	inline const ScoreComponentCollection &GetScoreComponent() const
	{
		return m_transScoreComponent;
	}
	inline const ScoreColl &GetLMScoreComponent() const
	{
		return m_lmScoreComponent;
	}
	inline const ScoreColl &GetGenerationScoreComponent() const
	{
		return m_generationScoreComponent;
	}
	
	void ResizeComponentScore(const LMList &allLM, const std::list < DecodeStep > &decodeStepList);
#endif
};

inline std::ostream& operator<<(std::ostream& out, const LatticeEdge& edge)
{
	out << edge.GetPhrase();
	return out;
}

