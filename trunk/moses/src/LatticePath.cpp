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

#include "LatticePath.h"
#include "LatticePathCollection.h"

using namespace std;

LatticePath::LatticePath(const Hypothesis *hypo)
:	m_prevEdgeChanged(NOT_FOUND)
{ // create path OF pure hypo
	m_scoreBreakdown					= hypo->GetScoreBreakdown();
	m_totalScore = hypo->GetTotalScore();

	// enumerate path using prevHypo
	while (hypo != NULL)
	{
		m_path.push_back(hypo);
		hypo = hypo->GetPrevHypo();
	}
}

LatticePath::LatticePath(const LatticePath &copy, size_t edgeIndex, const Hypothesis *arc)
:m_prevEdgeChanged(edgeIndex)
{
	for (size_t currEdge = 0 ; currEdge < edgeIndex ; currEdge++)
	{ // copy path from parent
		m_path.push_back(copy.m_path[currEdge]);
	}
	
	// 1 deviation
	m_path.push_back(arc);

	// rest of path comes from the deviation
	const Hypothesis *prevHypo = arc->GetPrevHypo();
	while (prevHypo != NULL)
	{
		m_path.push_back(prevHypo);
		prevHypo = prevHypo->GetPrevHypo();
	}

	CalcScore(copy, edgeIndex, arc);
}

LatticePath::LatticePath(const LatticePath &copy, size_t edgeIndex, const Hypothesis *arc, bool /*reserve*/)
:m_path(copy.m_path)
,m_prevEdgeChanged(edgeIndex)
{
	// 1 deviation
	m_path[edgeIndex] = arc;

	CalcScore(copy, edgeIndex, arc);
}

void LatticePath::CalcScore(const LatticePath &copy, size_t edgeIndex, const Hypothesis *arc)
{
	ScoreComponentCollection2 adj = arc->GetScoreBreakdown();
	adj.MinusEquals(copy.m_path[edgeIndex]->GetScoreBreakdown());
	m_scoreBreakdown = copy.m_scoreBreakdown;
	m_scoreBreakdown.PlusEquals(adj);	

	float fadj = arc->GetTotalScore() - copy.m_path[edgeIndex]->GetTotalScore();
	m_totalScore = copy.GetTotalScore() + fadj;
}

void LatticePath::CreateDeviantPaths(LatticePathCollection &pathColl) const
{
	const size_t sizePath = m_path.size();

	if (m_prevEdgeChanged == NOT_FOUND)
	{ // initial enumration from a pure hypo
		for (size_t currEdge = 0 ; currEdge < sizePath ; currEdge++)
		{
			const Hypothesis	*hypo		= static_cast<const Hypothesis*>(m_path[currEdge]);
			const ArcList *pAL = hypo->GetArcList();
      if (!pAL) continue;
			const ArcList &arcList = *pAL;

			// every possible Arc to replace this edge
			ArcList::const_iterator iterArc;
			for (iterArc = arcList.begin() ; iterArc != arcList.end() ; ++iterArc)
			{
				const Hypothesis *arc = *iterArc;
				LatticePath *deviantPath = new LatticePath(*this, currEdge, arc);
				pathColl.insert(deviantPath);
			}
		}
	}
	else
	{	// wiggle 1 of the edges only
		for (size_t currEdge = 0 ; currEdge < sizePath ; currEdge++)
		{
			if (currEdge != m_prevEdgeChanged)
			{
				const Hypothesis *edgeOrig = m_path[currEdge];
				const ArcList *pAL = m_path[currEdge]->GetArcList();
      	if (!pAL) continue;
				const ArcList &arcList = *pAL;
				ArcList::const_iterator iterArc;

				for (iterArc = arcList.begin() ; iterArc != arcList.end() ; ++iterArc)
				{	// copy this Path & change 1 edge
					const Hypothesis *arcReplace = *iterArc;

					if (arcReplace != edgeOrig && arcReplace->GetPrevHypo() == edgeOrig->GetPrevHypo())
					{
						LatticePath *deviantPath				= new LatticePath(*this, currEdge, arcReplace, true);
						pathColl.insert(deviantPath);
					}
				}
			}
		}
	}
}

TO_STRING_BODY(LatticePath);

