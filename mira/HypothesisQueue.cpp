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

#include <iostream>
#include "HypothesisQueue.h"

using namespace std;

namespace Moses
{

HypothesisQueue::~HypothesisQueue()
{
  m_queue.clear();
}

void HypothesisQueue::Push(BleuIndexPair hypo)
{
  //pair<set<BleuIndexPair>::iterator,bool> ret;

  if (m_capacity == 0 || m_queue.size() < m_capacity) {
    m_queue.insert(hypo);
  } else if (hypo.first > (*(m_queue.rbegin())).first) {
    // Remove the worst-scoring item from the queue and insert hypo (only erase item if new item was successfully added )
    /*ret = m_queue.insert(hypo);
    if ((*(ret.first)).second == 1) {
      HypoQueueType::iterator p = m_queue.end();
      --p;
      m_queue.erase(p);
      }*/
    // with multisets we do not have to check whether the item was successfully added
    m_queue.insert(hypo);
    HypoQueueType::iterator p = m_queue.end();
    --p;
    m_queue.erase(p);
  } else {
    // The hypo is unusable: the queue is full and hypo has a worse (or
    // equal) score than the worst-scoring item already held.
  }
}

BleuIndexPair HypothesisQueue::Pop()
{
  HypoQueueType::iterator p = m_queue.begin();
  BleuIndexPair top = *p;
  m_queue.erase(p);
  return top;
}

}  // namespace Moses
