// $Id: ChartTreillisPathMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:16 braunefe Exp $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "ChartTreillisPathMBOT.h"

#include "ChartHypothesisMBOT.h"
#include "ChartTreillisDetourMBOT.h"
#include "ChartTreillisDetourQueueMBOT.h"
#include "ChartTreillisNodeMBOT.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{
ChartTreillisPathMBOT::ChartTreillisPathMBOT(const ChartHypothesisMBOT &hypo)
    : m_mbotFinalNode(new ChartTreillisNodeMBOT(hypo))
    , m_mbotDeviationPoint(NULL)
    , m_mbotScoreBreakdown(hypo.GetScoreBreakdown())
    , m_mbotTotalScore(hypo.GetTotalScore())
{
}

ChartTreillisPathMBOT::ChartTreillisPathMBOT(const ChartTreillisDetourMBOT &detour)
   : m_mbotFinalNode(new ChartTreillisNodeMBOT(detour, m_mbotDeviationPoint))
   , m_mbotScoreBreakdown(detour.GetBasePathMBOT().m_mbotScoreBreakdown)
   , m_mbotTotalScore(0)
{
  CHECK(m_mbotDeviationPoint);
  ScoreComponentCollection scoreChange;
  scoreChange = detour.GetReplacementHypoMBOT().GetScoreBreakdown();
  scoreChange.MinusEquals(detour.GetSubstitutedNodeMBOT().GetHypothesisMBOT().GetScoreBreakdown());
  m_mbotScoreBreakdown.PlusEquals(scoreChange);
  m_mbotTotalScore = m_mbotScoreBreakdown.GetWeightedScore();
}

ChartTreillisPathMBOT::~ChartTreillisPathMBOT()
{
  delete m_mbotFinalNode;
}

Phrase ChartTreillisPathMBOT::GetOutputPhraseMBOT(ProcessedNonTerminals * pnt) const
{
  Phrase ret = GetFinalNodeMBOT().GetOutputPhrase(pnt);
  return ret;
}

}  // namespace Moses
