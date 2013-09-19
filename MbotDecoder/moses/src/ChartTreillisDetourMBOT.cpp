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

#include "ChartTreillisDetourMBOT.h"

#include "ChartHypothesisMBOT.h"
#include "ChartTreillisNodeMBOT.h"
#include "ChartTreillisPathMBOT.h"

namespace Moses
{

ChartTreillisDetourMBOT::ChartTreillisDetourMBOT(
    boost::shared_ptr<const ChartTreillisPathMBOT> basePath,
    const ChartTreillisNodeMBOT &substitutedNode,
    const ChartHypothesisMBOT &replacementHypo)
  : m_mbotBasePath(basePath)
  , m_mbotSubstitutedNode(substitutedNode)
  , m_mbotReplacementHypo(replacementHypo)
{
  float diff = replacementHypo.GetTotalScore()
             - substitutedNode.GetHypothesisMBOT().GetTotalScore();
  m_mbotTotalScore = basePath->GetTotalScoreMBOT() + diff;
}

}  // namespace Moses
