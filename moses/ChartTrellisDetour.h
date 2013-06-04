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

namespace Moses
{
class ChartHypothesis;
class ChartTrellisNode;
class ChartTrellisPath;

/** @todo Something to do with make deviant paths
 */
class ChartTrellisDetour
{
public:
  ChartTrellisDetour(boost::shared_ptr<const ChartTrellisPath>,
                     const ChartTrellisNode &, const ChartHypothesis &);

  const ChartTrellisPath &GetBasePath() const {
    return *m_basePath;
  }
  const ChartTrellisNode &GetSubstitutedNode() const {
    return m_substitutedNode;
  }
  const ChartHypothesis &GetReplacementHypo() const {
    return m_replacementHypo;
  }
  float GetTotalScore() const {
    return m_totalScore;
  }

private:
  boost::shared_ptr<const ChartTrellisPath> m_basePath;
  const ChartTrellisNode &m_substitutedNode;
  const ChartHypothesis &m_replacementHypo;
  float m_totalScore;
};

}  // namespace Moses
