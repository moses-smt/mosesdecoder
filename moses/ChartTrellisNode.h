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

#include <vector>
#include "Phrase.h"

namespace Moses
{
class ScoreComponentCollection;
class ChartHypothesis;
class ChartTrellisDetour;

/**  1 node in the output hypergraph. Used in ChartTrellisPath
 */
class ChartTrellisNode
{
public:
  typedef std::vector<ChartTrellisNode*> NodeChildren;

  ChartTrellisNode(const ChartHypothesis &hypo);
  ChartTrellisNode(const ChartTrellisDetour &, ChartTrellisNode *&);

  ~ChartTrellisNode();

  const ChartHypothesis &GetHypothesis() const {
    return m_hypo;
  }

  const NodeChildren &GetChildren() const {
    return m_children;
  }

  const ChartTrellisNode &GetChild(size_t i) const {
    return *m_children[i];
  }

  Phrase GetOutputPhrase() const;

private:
  ChartTrellisNode(const ChartTrellisNode &);  // Not implemented
  ChartTrellisNode& operator=(const ChartTrellisNode &);  // Not implemented

  ChartTrellisNode(const ChartTrellisNode &, const ChartTrellisNode &,
                   const ChartHypothesis &, ChartTrellisNode *&);

  void CreateChildren();
  void CreateChildren(const ChartTrellisNode &, const ChartTrellisNode &,
                      const ChartHypothesis &, ChartTrellisNode *&);

  const ChartHypothesis &m_hypo;
  NodeChildren m_children;
};

}
