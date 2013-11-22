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

#include "RuleCubeItem.h"

#include <boost/functional/hash.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

#include "util/exception.hh"
#include <queue>
#include <set>
#include <vector>

namespace Moses
{

class ChartCellCollection;
class ChartManager;
class ChartTranslationOptions;

/** Define an ordering between RuleCubeItems based on their scores.
 * This is used to order items in the cube's priority queue.
 */
class RuleCubeItemScoreOrderer
{
public:
  bool operator()(const RuleCubeItem *p, const RuleCubeItem *q) const {
    return p->GetScore() < q->GetScore();
  }
};

/** Define an ordering between RuleCubeItems based on their positions in the
 * cube.  This is used to record which positions in the cube have been covered
 * during search.
 */
class RuleCubeItemPositionOrderer
{
public:
  bool operator()(const RuleCubeItem *p, const RuleCubeItem *q) const {
    return *p < *q;
  }
};

/** @todo what is this?
 */
class RuleCubeItemHasher
{
public:
  size_t operator()(const RuleCubeItem *p) const {
    size_t seed = 0;
    boost::hash_combine(seed, p->GetHypothesisDimensions());
    boost::hash_combine(seed, p->GetTranslationDimension().GetTranslationOption());
    return seed;
  }
};

/** @todo what is this?
 */
class RuleCubeItemEqualityPred
{
public:
  bool operator()(const RuleCubeItem *p, const RuleCubeItem *q) const {
    return p->GetHypothesisDimensions() == q->GetHypothesisDimensions() &&
           p->GetTranslationDimension() == q->GetTranslationDimension();
  }
};

/** @todo what is this?
 */
class RuleCube
{
public:
  RuleCube(const ChartTranslationOptions &, const ChartCellCollection &,
           ChartManager &);

  ~RuleCube();

  float GetTopScore() const {
	UTIL_THROW_IF2(m_queue.empty(), "Empty queue, nothing to pop");
    RuleCubeItem *item = m_queue.top();
    return item->GetScore();
  }

  RuleCubeItem *Pop(ChartManager &);

  bool IsEmpty() const {
    return m_queue.empty();
  }

  const ChartTranslationOptions &GetTranslationOption() const {
    return m_transOpt;
  }

private:
#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_set<RuleCubeItem*,
          RuleCubeItemHasher,
          RuleCubeItemEqualityPred
          > ItemSet;
#else
  typedef std::set<RuleCubeItem*, RuleCubeItemPositionOrderer> ItemSet;
#endif

  typedef std::priority_queue<RuleCubeItem*,
          std::vector<RuleCubeItem*>,
          RuleCubeItemScoreOrderer
          > Queue;

  RuleCube(const RuleCube &);  // Not implemented
  RuleCube &operator=(const RuleCube &);  // Not implemented

  void CreateNeighbors(const RuleCubeItem &, ChartManager &);
  void CreateNeighbor(const RuleCubeItem &, int, ChartManager &);

  const ChartTranslationOptions &m_transOpt;
  ItemSet m_covered;
  Queue m_queue;
};

}
