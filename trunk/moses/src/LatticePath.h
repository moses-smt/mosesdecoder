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
#include "Hypothesis.h"
#include "TypeDef.h"

class LatticePathCollection;

class LatticePath
{
	friend std::ostream& operator<<(std::ostream&, const LatticePath&);

protected:
	std::vector<const Hypothesis *> m_path;
	size_t		m_prevEdgeChanged;

	ScoreComponentCollection2	m_scoreBreakdown;
	float m_totalScore;
 
	void CalcScore(const LatticePath &copy, size_t edgeIndex, const Hypothesis *arc);

public:
	LatticePath(); // not implemented
	
	LatticePath(const Hypothesis *hypo);
		// create path OF pure hypo
	LatticePath(const LatticePath &copy, size_t edgeIndex, const Hypothesis *arc);
		// create path FROM pure hypo
		// deviate from edgeIndex backwards
	LatticePath(const LatticePath &copy, size_t edgeIndex, const Hypothesis *arc, bool reserve);
		// create path from ANY hypo
		// reserve arg not used. to differential from other constructor
		// deviate from edgeIndex. however, all other edges the same

	inline float GetTotalScore() const { return m_totalScore; }

	inline const std::vector<const Hypothesis *> &GetEdges() const
	{
		return m_path;
	}

	inline bool IsPurePath() const
	{
		return m_prevEdgeChanged == NOT_FOUND;
	}

#ifdef N_BEST
	void CreateDeviantPaths(LatticePathCollection &pathColl) const;

	inline const ScoreComponentCollection2 &GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

#endif

	TO_STRING;

};

// friend
inline std::ostream& operator<<(std::ostream& out, const LatticePath& path)
{
	const size_t sizePath = path.m_path.size();
	for (int pos = (int) sizePath - 1 ; pos >= 0 ; pos--)
	{
		const Hypothesis *edge = path.m_path[pos];
		out << *edge;
	}
	// scores
	out << " total=" << path.GetTotalScore();
#ifdef N_BEST
	out << " " << path.GetScoreBreakdown();
#endif

	return out;
}
