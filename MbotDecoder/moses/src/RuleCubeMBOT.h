// $Id: RuleCubeMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#include "RuleCubeItemMBOT.h"

#include <boost/functional/hash.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

#include "util/check.hh"
#include <queue>
#include <set>
#include <vector>

namespace Moses
{

class ChartCellCollection;
class ChartManager;
class ChartTranslationOptionMBOT;

// Define an ordering between RuleCubeItems based on their scores.  This
// is used to order items in the cube's priority queue.
class RuleCubeItemScoreOrdererMBOT //: public RuleCubeItemScoreOrderer
{
 public:
  bool operator()(const RuleCubeItemMBOT *p, const RuleCubeItemMBOT *q) const {
    return p->GetScoreMBOT() < q->GetScoreMBOT();
  }
};

// Define an ordering between RuleCubeItems based on their positions in the
// cube.  This is used to record which positions in the cube have been covered
// during search.
class RuleCubeItemPositionOrdererMBOT //: public RuleCubeItemPositionOrderer
{
 public:
  bool operator()(const RuleCubeItemMBOT *p, const RuleCubeItemMBOT *q) const {
    return *p < *q;
  }
};

//BEWARE : We don't use BOOST for now but could be useful
/*class RuleCubeItemHasherMBOT //: public RuleCubeItemHasher
{
 public:
  size_t operator()(const RuleCubeItemMBOT *p) const {
    std::cout << "RCMBOT : Making Hash"<< std::endl;
    size_t seed = 0;
    std::cout << "RCMBOT : hash combine..." << std::endl;
    boost::hash_combine(seed, p->GetHypothesisDimensionsMBOT());
    std::cout << "RCMBOT : hash combine done" << std::endl;
    boost::hash_combine(seed, p->GetTranslationDimensionMBOT().GetTargetPhrase());
    std::cout << "RCMBOT : Hash done. Return Seed."<< std::endl;
    return seed;
  }
};*/

class RuleCubeItemEqualityPredMBOT //: public RuleCubeItemEqualityPred
{
 public:
  bool operator()(const RuleCubeItemMBOT *p, const RuleCubeItemMBOT *q) const {
      //std::cout << "RCMBOT : Making Equality Pred "<< std::endl;
    return p->GetHypothesisDimensionsMBOT() == q->GetHypothesisDimensionsMBOT() &&
           p->GetTranslationDimensionMBOT() == q->GetTranslationDimensionMBOT();
  }
};

class RuleCubeMBOT : public RuleCube
{
 public:
  RuleCubeMBOT(const ChartTranslationOptionMBOT &, const ChartCellCollection &,
           ChartManager &);

  ~RuleCubeMBOT();

  float GetTopScore() const {
      std::cout << "Get top score NOT IMPLEMENTED for non-mbot queue in RuleCubeMBOT" << std::endl;
  }

   float GetTopScoreMBOT() const {
    CHECK(!m_mbotQueue.empty());
    RuleCubeItemMBOT *item = m_mbotQueue.top();
    return item->GetScoreMBOT();
  }

  RuleCubeItem *Pop(ChartManager &)
  {
        std::cout << "Pop NOT IMPLEMENTED for non-mbot queue in RuleCubeMBOT" << std::endl;
  }

  RuleCubeItemMBOT *PopMBOT(ChartManager &);

  bool IsEmpty() const {
        std::cout << "Emptiness test NOT IMPLEMENTED for non-mbot queue in RuleCubeMBOT" << std::endl;
      }


  bool IsEmptyMBOT() const { return m_mbotQueue.empty(); }

  const ChartTranslationOption &GetTranslationOption() const {
       std::cout << "Get translation option NOT IMPLEMENTED for non-mbot queue in RuleCubeMBOT" << std::endl;
  }

  const ChartTranslationOptionMBOT &GetTranslationOptionMBOT() const {
    return m_mbotTransOpt;
  }

 private:
//! removed Boost
/*if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_set<RuleCubeItemMBOT*,
                               RuleCubeItemHasherMBOT,
                               RuleCubeItemEqualityPredMBOT
                              > ItemSetMBOT;
#else*/
  typedef std::set<RuleCubeItemMBOT*, RuleCubeItemPositionOrdererMBOT> ItemSetMBOT;
//#endif

  typedef std::priority_queue<RuleCubeItemMBOT*,
                              std::vector<RuleCubeItemMBOT*>,
                              RuleCubeItemScoreOrdererMBOT
                             > QueueMBOT;

  RuleCubeMBOT(const RuleCubeMBOT &);  // Not implemented
  RuleCubeMBOT &operator=(const RuleCubeMBOT &);  // Not implemented

  void CreateNeighborsMBOT(const RuleCubeItemMBOT &, ChartManager &);
  void CreateNeighborMBOT(const RuleCubeItemMBOT &, int, ChartManager &);

  const ChartTranslationOptionMBOT &m_mbotTransOpt;
  ItemSetMBOT m_mbotCovered;
  QueueMBOT m_mbotQueue;
};

}
