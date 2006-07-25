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
#include "LatticeEdge.h"
#include "ScoreComponent.h"

class Arc : public LatticeEdge
{
public:
	const Hypothesis *m_mainHypo;

	Arc(const Arc &arc); // not implemented
	
	Arc( const float score[]
			, const ScoreComponentCollection 	&transScoreComponent
			, const ScoreColl					 						&lmScoreComponent
			, const ScoreColl											&generationScoreColl
			, const Phrase												&phrase
			, const Hypothesis 										*prevHypo) 
		:LatticeEdge(score, transScoreComponent, lmScoreComponent, generationScoreColl, phrase, prevHypo)
	{
	}

	inline void SetMainHypo(const Hypothesis &hypo)
	{
		m_mainHypo = &hypo;
	}
	~Arc();

	const std::vector<Arc*>* GetArcList() const;
};

std::ostream& operator<<(std::ostream& out, const Arc& arc);
