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

#include "ChartTrellisDetourQueue.h"

#include "Util.h"

namespace Moses
{

ChartTrellisDetourQueue::~ChartTrellisDetourQueue()
{
  RemoveAllInColl(m_queue);
}

void ChartTrellisDetourQueue::Push(const ChartTrellisDetour *detour)
{
  if (m_capacity == 0 || m_queue.size() < m_capacity) {
    m_queue.insert(detour);
  } else if (detour->GetTotalScore() > (*m_queue.rbegin())->GetTotalScore()) {
    // Remove the worst-scoring item from the queue and insert detour.
    QueueType::iterator p = m_queue.end();
    delete *--p;
    m_queue.erase(p);
    m_queue.insert(detour);
  } else {
    // The detour is unusable: the queue is full and detour has a worse (or
    // equal) score than the worst-scoring item already held.
    delete detour;
  }
}

const ChartTrellisDetour *ChartTrellisDetourQueue::Pop()
{
  QueueType::iterator p = m_queue.begin();
  const ChartTrellisDetour *top = *p;
  m_queue.erase(p);
  return top;
}

}  // namespace Moses
