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

#include "ChartTrellisNode.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

namespace Moses
{
class ChartHypothesis;
class ChartTrellisPathCollection;

class ChartTrellisPath
{
  friend std::ostream& operator<<(std::ostream&, const ChartTrellisPath&);

protected:
  // recursively point backwards
  ChartTrellisNode *m_finalNode;
  const ChartTrellisNode *m_prevNodeChanged;
  const ChartTrellisPath *m_prevPath;

  Moses::ScoreComponentCollection	m_scoreBreakdown;
  float m_totalScore;

  // deviate by 1 hypo
  ChartTrellisPath(const ChartTrellisPath &origPath
              , const ChartTrellisNode &soughtNode
              , const ChartHypothesis &replacementHypo
              , Moses::ScoreComponentCollection	&scoreChange);

  void CreateDeviantPaths(ChartTrellisPathCollection &pathColl, const ChartTrellisNode &soughtNode) const;

  const ChartTrellisNode &GetFinalNode() const {
    assert (m_finalNode);
    return *m_finalNode;
  }

public:
  ChartTrellisPath(const ChartHypothesis *hypo);
  ~ChartTrellisPath();

  //! get score for this path throught trellis
  inline float GetTotalScore() const {
    return m_totalScore;
  }

  Moses::Phrase GetOutputPhrase() const;

  /** returns detailed component scores */
  inline const Moses::ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }

  void CreateDeviantPaths(ChartTrellisPathCollection &pathColl) const;
};


}

