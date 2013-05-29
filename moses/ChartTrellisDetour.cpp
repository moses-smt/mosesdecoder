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

#include "ChartTrellisDetour.h"

#include "ChartHypothesis.h"
#include "ChartTrellisNode.h"
#include "ChartTrellisPath.h"

namespace Moses
{

ChartTrellisDetour::ChartTrellisDetour(
  boost::shared_ptr<const ChartTrellisPath> basePath,
  const ChartTrellisNode &substitutedNode,
  const ChartHypothesis &replacementHypo)
  : m_basePath(basePath)
  , m_substitutedNode(substitutedNode)
  , m_replacementHypo(replacementHypo)
{
  float diff = replacementHypo.GetTotalScore()
               - substitutedNode.GetHypothesis().GetTotalScore();
  m_totalScore = basePath->GetTotalScore() + diff;
}

}  // namespace Moses
