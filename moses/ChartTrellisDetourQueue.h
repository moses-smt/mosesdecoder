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

#include "ChartTrellisDetour.h"

#include <set>

namespace Moses
{

/** A bounded priority queue of ChartTrellisDetour pointers.  The top item is
 *  the best scoring detour.  The queue assumes ownership of pushed items and
 *  relinquishes ownership when they are popped.  Any remaining items at the
 *  time of the queue's destruction are deleted.
 */
class ChartTrellisDetourQueue
{
public:
  // Create empty queue with fixed capacity of c.  Capacity 0 means unbounded.
  ChartTrellisDetourQueue(size_t c) : m_capacity(c) {}
  ~ChartTrellisDetourQueue();

  bool Empty() const {
    return m_queue.empty();
  }

  // Add the detour to the queue or delete it if the queue is full and the
  // score is no better than the queue's worst score.
  void Push(const ChartTrellisDetour *detour);

  // Remove the best-scoring detour from the queue and return it.  The
  // caller is responsible for deleting the object.
  const ChartTrellisDetour *Pop();

private:
  struct DetourOrderer {
    bool operator()(const ChartTrellisDetour* a,
                    const ChartTrellisDetour* b) const {
      return (a->GetTotalScore() > b->GetTotalScore());
    }
  };

  typedef std::multiset<const ChartTrellisDetour *, DetourOrderer> QueueType;

  QueueType m_queue;
  const size_t m_capacity;
};

}  // namespace Moses
