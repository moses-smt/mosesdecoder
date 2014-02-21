// $Id$
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

#include "ChartTrellisPath.h"

#include "ChartHypothesis.h"
#include "ChartTrellisDetour.h"
#include "ChartTrellisDetourQueue.h"
#include "ChartTrellisNode.h"
#include "util/exception.hh"

namespace Moses
{

ChartTrellisPath::ChartTrellisPath(const ChartHypothesis &hypo)
  : m_finalNode(new ChartTrellisNode(hypo))
  , m_deviationPoint(NULL)
  , m_scoreBreakdown(hypo.GetScoreBreakdown())
  , m_totalScore(hypo.GetTotalScore())
{
}

ChartTrellisPath::ChartTrellisPath(const ChartTrellisDetour &detour)
  : m_finalNode(new ChartTrellisNode(detour, m_deviationPoint))
  , m_scoreBreakdown(detour.GetBasePath().m_scoreBreakdown)
  , m_totalScore(0)
{
  UTIL_THROW_IF2(m_deviationPoint == NULL, "No deviation point");
  ScoreComponentCollection scoreChange;
  scoreChange = detour.GetReplacementHypo().GetScoreBreakdown();
  scoreChange.MinusEquals(detour.GetSubstitutedNode().GetHypothesis().GetScoreBreakdown());
  m_scoreBreakdown.PlusEquals(scoreChange);
  m_totalScore = m_scoreBreakdown.GetWeightedScore();
}

ChartTrellisPath::~ChartTrellisPath()
{
  delete m_finalNode;
}

Phrase ChartTrellisPath::GetOutputPhrase() const
{
  Phrase ret = GetFinalNode().GetOutputPhrase();
  return ret;
}

}  // namespace Moses
