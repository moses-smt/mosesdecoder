// $Id: ChartTreillisNodeMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#include "ChartTrellisNode.h"
#include "ProcessedNonTerminals.h"

namespace Moses
{
class ScoreComponentCollection;
class ChartHypothesisMBOT;
class ChartTreillisDetourMBOT;

class ChartTreillisNodeMBOT
{
 public:
  typedef std::vector<ChartTreillisNodeMBOT*> NodeChildrenMBOT;

  ChartTreillisNodeMBOT(const ChartHypothesisMBOT &hypo);
  ChartTreillisNodeMBOT(const ChartTreillisDetourMBOT &, ChartTreillisNodeMBOT *&);

  ~ChartTreillisNodeMBOT();

  const ChartHypothesisMBOT &GetHypothesisMBOT() const { return m_mbotHypo; }

  const NodeChildrenMBOT &GetChildrenMBOT() const {
    return m_mbotChildren; }

  const ChartTreillisNodeMBOT &GetChildMBOT(size_t i) const { return *m_mbotChildren[i]; }

  Phrase GetOutputPhrase(ProcessedNonTerminals * pnt) const;

 private:
  ChartTreillisNodeMBOT(const ChartTreillisNodeMBOT &);  // Not implemented
  ChartTreillisNodeMBOT& operator=(const ChartTreillisNodeMBOT &);  // Not implemented

  ChartTreillisNodeMBOT(const ChartTreillisNodeMBOT &, const ChartTreillisNodeMBOT &,
                   const ChartHypothesisMBOT &, ChartTreillisNodeMBOT *&);

  void CreateChildrenMBOT();
  void CreateChildrenMBOT(const ChartTreillisNodeMBOT &, const ChartTreillisNodeMBOT &,
                      const ChartHypothesisMBOT &, ChartTreillisNodeMBOT *&);

  const ChartHypothesisMBOT &m_mbotHypo;
  NodeChildrenMBOT m_mbotChildren;
};

}
