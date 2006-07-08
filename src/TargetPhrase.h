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

#include <vector>
#include "Phrase.h"
#include "TransScoreComponent.h"

class PhraseDictionary;

class TargetPhrase: public Phrase
{
protected:
	float m_score;
#ifdef N_BEST
	TransScoreComponent m_scoreComponent;
#endif

public:
	TargetPhrase(FactorDirection direction, const PhraseDictionary *phraseDictionary)
		:Phrase(direction)
#ifdef N_BEST
		,m_scoreComponent(phraseDictionary)
#endif
	{
	}
	float GetScore() const
	{
		return m_score;
	}
	void SetScore(const std::vector<float> &scoreVector, const std::vector<float> &weightT);
	void ResetScore();
	void SetWeight(const std::vector<float> &weightT);

#ifdef N_BEST
	inline const TransScoreComponent &GetScoreComponents() const
	{
		return m_scoreComponent;
	}
#endif

};

