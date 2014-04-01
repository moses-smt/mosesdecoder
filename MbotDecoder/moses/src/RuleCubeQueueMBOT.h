// $Id: RuleCubeQueueMBOT.h,v 1.1.1.1 2013/01/06 16:54:16 braunefe Exp $
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "RuleCubeQueue.h"
#include "RuleCubeMBOT.h"

#include <queue>
#include <vector>

namespace Moses
{

class ChartManager;

//! Add rule cube ordered for mbot rule cubes
class RuleCubeOrdererMBOT : public RuleCubeOrderer
{
 public:
  bool operator()(const RuleCubeMBOT *p, const RuleCubeMBOT *q) const {
    return p->GetTopScoreMBOT() < q->GetTopScoreMBOT();
  }
};

//! Add rule cube ordered for mbot rule cubes
class RuleCubeQueueMBOT : public RuleCubeQueue
{
 public:
  RuleCubeQueueMBOT(ChartManager &manager) :
  RuleCubeQueue(manager),m_manager(manager) {}
  ~RuleCubeQueueMBOT();

  //! forbid adding non mbot RuleCube
  void Add(RuleCube *)
  {
      std::cout << "Adding non mbot Rule Cube NOT IMPLEMENTED in Rule Cube MBOT" << std::endl;
  }

  void Add(RuleCubeMBOT *);

  //! forbid popping non mbot chart hypotheses
  ChartHypothesis *Pop()
  {
      std::cout << "Popping non mbot queue NOT IMPLEMENTED in Rule Cube MBOT" << std::endl;
  }

  ChartHypothesisMBOT *PopMBOT();

  bool IsEmpty() const
  {
       std::cout << "Emptiness test on non mbot queue NOT IMPLEMENTED in Rule Cube MBOT" << std::endl;
  }

  bool IsEmptyMBOT() const { return m_mbotQueue.empty(); }

 private:
  typedef std::priority_queue<RuleCubeMBOT*, std::vector<RuleCubeMBOT*>,
                              RuleCubeOrdererMBOT > QueueMBOT;

  QueueMBOT m_mbotQueue;
  ChartManager &m_manager;
};
}
