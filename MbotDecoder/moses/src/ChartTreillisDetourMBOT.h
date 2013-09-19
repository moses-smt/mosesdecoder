/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include <boost/shared_ptr.hpp>

#include "ChartTrellisDetour.h"

namespace Moses
{

class ChartHypothesisMBOT;
class ChartTreillisNodeMBOT;
class ChartTreillisPathMBOT;

class ChartTreillisDetourMBOT
{

 public:
  ChartTreillisDetourMBOT(boost::shared_ptr<const ChartTreillisPathMBOT>,
                     const ChartTreillisNodeMBOT &, const ChartHypothesisMBOT &);

  const ChartTreillisPathMBOT &GetBasePathMBOT() const { return *m_mbotBasePath; }

  const ChartTreillisNodeMBOT &GetSubstitutedNodeMBOT() const {
    return m_mbotSubstitutedNode;
  }

  const ChartHypothesisMBOT &GetReplacementHypoMBOT() const {
    return m_mbotReplacementHypo;
  }

  float GetTotalScoreMBOT() const { return m_mbotTotalScore; }

 private:
  boost::shared_ptr<const ChartTreillisPathMBOT> m_mbotBasePath;
  const ChartTreillisNodeMBOT &m_mbotSubstitutedNode;
  const ChartHypothesisMBOT &m_mbotReplacementHypo;
  float m_mbotTotalScore;
};

}  // namespace Moses
