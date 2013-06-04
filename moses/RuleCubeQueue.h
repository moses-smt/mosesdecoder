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

#include "RuleCube.h"

#include <queue>
#include <vector>

namespace Moses
{

class ChartManager;

/** Define an ordering between RuleCube based on their best item scores.  This
 * is used to order items in the priority queue.
 */
class RuleCubeOrderer
{
public:
  bool operator()(const RuleCube *p, const RuleCube *q) const {
    return p->GetTopScore() < q->GetTopScore();
  }
};

/** @todo how is this used */
class RuleCubeQueue
{
public:
  RuleCubeQueue(ChartManager &manager) : m_manager(manager) {}
  ~RuleCubeQueue();

  void Add(RuleCube *);
  ChartHypothesis *Pop();
  bool IsEmpty() const {
    return m_queue.empty();
  }

private:
  typedef std::priority_queue<RuleCube*, std::vector<RuleCube*>,
          RuleCubeOrderer > Queue;

  Queue m_queue;
  ChartManager &m_manager;
};

}
