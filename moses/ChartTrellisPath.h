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

#pragma once

#include "ScoreComponentCollection.h"
#include "Phrase.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{

class ChartHypothesis;
class ChartTrellisDetour;
class ChartTrellisDetourQueue;
class ChartTrellisNode;

/** 1 path through the output hypergraph
 *  The class hold the final node in the path used for constructing n-best list in chart decoding.
 *  Each node hold it's own children.
 *  Also contains the total score and score breakdown for this path.
 */
class ChartTrellisPath
{
public:
  ChartTrellisPath(const ChartHypothesis &hypo);
  ChartTrellisPath(const ChartTrellisDetour &detour);

  ~ChartTrellisPath();

  const ChartTrellisNode &GetFinalNode() const {
    return *m_finalNode;
  }

  const ChartTrellisNode *GetDeviationPoint() const {
    return m_deviationPoint;
  }

  //! get score for this path throught trellis
  float GetTotalScore() const {
    return m_totalScore;
  }

  Phrase GetOutputPhrase() const;

  /** returns detailed component scores */
  const ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }

private:
  ChartTrellisPath(const ChartTrellisPath &);  // Not implemented
  ChartTrellisPath &operator=(const ChartTrellisPath &);  // Not implemented

  ChartTrellisNode *m_finalNode;
  ChartTrellisNode *m_deviationPoint;
  ScoreComponentCollection m_scoreBreakdown;
  float m_totalScore;
};

}  // namespace Moses
