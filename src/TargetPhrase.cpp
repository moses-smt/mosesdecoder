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

#include <assert.h>
#include "TargetPhrase.h"
#include "PhraseDictionary.h"

using namespace std;

TargetPhrase::TargetPhrase(FactorDirection direction, const PhraseDictionary *phraseDictionary)
:Phrase(direction)
#ifdef N_BEST
	,m_scoreComponent(phraseDictionary)
#endif
{
}

void TargetPhrase::SetScore(const vector<float> &scoreVector, const vector<float> &weightT)
{
	assert(weightT.size() == scoreVector.size());

	// calc average score if non-best
	m_score = 0;
	for (size_t i = 0 ; i < scoreVector.size() ; i++)
	{
		float score =  TransformScore(scoreVector[i]);
#ifdef N_BEST
		m_scoreComponent[i] = score;
#endif
		m_score += score * weightT[i];
	}
}

void TargetPhrase::SetWeight(const vector<float> &weightT)
{
#ifdef N_BEST
	m_score = 0;
	for (size_t i = 0 ; i < weightT.size() ; i++)
	{
		m_score += m_scoreComponent[i] * weightT[i];
	}
#endif
}

void TargetPhrase::ResetScore()
{
	m_score = 0;
#ifdef N_BEST
	m_scoreComponent.Reset();
#endif
}

