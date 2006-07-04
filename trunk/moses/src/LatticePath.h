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
#include <vector>
#include <limits>
#include "LatticeEdge.h"
#include "Hypothesis.h"
#include "TypeDef.h"
#include "TransScoreComponent.h"
#include "ScoreColl.h"

class Arc;
class LatticePathCollection;

class LatticePath
{
	friend std::ostream& operator<<(std::ostream&, const LatticePath&);

protected:
	std::vector<const LatticeEdge*> m_path;
	size_t		m_prevEdgeChanged;
	float			m_score[NUM_SCORES];

	TransScoreComponentCollection	m_transScoreComponent;
	ScoreColl											m_generationScoreComponent
																, m_lmScoreComponent;
 
	void CalcScore(const LatticePath &copy, size_t edgeIndex, const Arc *arc);

public:
	LatticePath(); // not implemented
	
	LatticePath(const Hypothesis *hypo);
		// create path OF pure hypo
	LatticePath(const LatticePath &copy, size_t edgeIndex, const Arc *arc);
		// create path FROM pure hypo
		// deviate from edgeIndex backwards
	LatticePath(const LatticePath &copy, size_t edgeIndex, const Arc *arc, bool reserve);
		// create path from ANY hypo
		// reserve arg not used. to differential from other constructor
		// deviate from edgeIndex. however, all other edges the same

	inline float GetScore(ScoreType scoreType) const
	{
		return m_score[scoreType];
	}
	inline const std::vector<const LatticeEdge*> &GetEdges() const
	{
		return m_path;
	}

	inline bool IsPurePath() const
	{
		return m_prevEdgeChanged == NOT_FOUND;
	}

#ifdef N_BEST
	void CreateDeviantPaths(LatticePathCollection &pathColl) const;

	inline const ScoreColl &GetLMScoreComponent() const
	{
		return m_lmScoreComponent;
	}
	inline float GetLMScoreComponent(size_t index) const
	{
		return m_lmScoreComponent.GetValue(index);
	}
	inline const TransScoreComponentCollection &GetTransScoreComponent() const
	{
		return m_transScoreComponent;
	}
	inline const ScoreColl &GetGenerationScoreComponent() const
	{
		return m_generationScoreComponent;
	}

#endif

};

// friend
inline std::ostream& operator<<(std::ostream& out, const LatticePath& path)
{
	const size_t sizePath = path.m_path.size();
	for (int pos = (int) sizePath - 1 ; pos >= 0 ; pos--)
	{
		const LatticeEdge *edge = path.m_path[pos];
		out << *edge;
	}
	// scores
	out << " [" << path.GetScore( static_cast<ScoreType>(0));
	for (size_t i = 1 ; i < NUM_SCORES ; i++)
	{
		out << "," << path.GetScore( static_cast<ScoreType>(i));
	}
	out << "]";
#ifdef N_BEST
	out << " " << path.GetTransScoreComponent();
	out << " " << path.GetGenerationScoreComponent();
#endif

	return out;
}
