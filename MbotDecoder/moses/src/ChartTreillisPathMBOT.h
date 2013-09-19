// $Id: ChartTreillisPathMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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
#include "ChartTrellisPath.h"
#include "ProcessedNonTerminals.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{

class ChartHypothesisMBOT;
class ChartTreillisDetourMBOT;
class ChartTreillisDetourQueueMBOT;
class ChartTreillisNodeMBOT;

class ChartTreillisPathMBOT
{
 public:
  ChartTreillisPathMBOT(const ChartHypothesisMBOT &hypo);
  ChartTreillisPathMBOT(const ChartTreillisDetourMBOT &detour);

  ~ChartTreillisPathMBOT();

  const ChartTreillisNodeMBOT &GetFinalNodeMBOT() const { return *m_mbotFinalNode; }

  const ChartTreillisNodeMBOT *GetDeviationPointMBOT() const { return m_mbotDeviationPoint; }

  //! get score for this path throught trellis
  float GetTotalScoreMBOT() const { return m_mbotTotalScore; }

  Phrase GetOutputPhraseMBOT(ProcessedNonTerminals * nt) const;

  /** returns detailed component scores */
  const ScoreComponentCollection &GetScoreBreakdownMBOT() const {
    return m_mbotScoreBreakdown;
  }

 private:
  ChartTreillisPathMBOT(const ChartTreillisPathMBOT &);  // Not implemented
  ChartTreillisPathMBOT &operator=(const ChartTreillisPathMBOT &);  // Not implemented

  //BEWARE ALSO CONTAINTS MBOT ChartTreillisNodes
  ChartTreillisNodeMBOT *m_mbotFinalNode;
  ChartTreillisNodeMBOT *m_mbotDeviationPoint;

  ScoreComponentCollection m_mbotScoreBreakdown;
  float m_mbotTotalScore;
};

}  // namespace Moses
