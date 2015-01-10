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

#include <set>

namespace Moses
{

// pair of Bleu score and index
typedef std::pair<float, size_t> BleuIndexPair;

// A bounded priority queue of BleuIndexPairs. The top item is
// the best scoring hypothesis.  The queue assumes ownership of pushed items and
// relinquishes ownership when they are popped.  Any remaining items at the
// time of the queue's destruction are deleted.
class HypothesisQueue
{

public:
  // Create empty queue with fixed capacity of c.  Capacity 0 means unbounded.
  HypothesisQueue(size_t c) : m_capacity(c) {}
  ~HypothesisQueue();

  bool Empty() {
    return m_queue.empty();
  }

  // Add the hypo to the queue or delete it if the queue is full and the
  // score is no better than the queue's worst score.
  void Push(BleuIndexPair hypo);

  // Remove the best-scoring detour from the queue and return it.  The
  // caller is responsible for deleting the object.
  BleuIndexPair Pop();

private:
  struct HypothesisOrderer {
    bool operator()(BleuIndexPair a,
                    BleuIndexPair b) {
      return (a.first > b.first);
    }
  };

  typedef std::multiset<BleuIndexPair, HypothesisOrderer> HypoQueueType;
  //typedef std::set<BleuIndexPair, HypothesisOrderer> HypoQueueType;

  HypoQueueType m_queue;
  const size_t m_capacity;
};

}  // namespace Moses
