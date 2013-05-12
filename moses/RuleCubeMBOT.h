//Fabienne Braune
//Rule Cube for dealing with RuleCubeItems containing l-MBOT hypotheses
//More information in class RuleCubeItemMBOT.

//Comments on implementation : could use RuleCube and implement a method creating an l-MBOT RuleCubeItem (RuleCubeItemMBOT)
//Found it more modular to write a derived class but this can be easily modified

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

class RuleCubeItemEqualityPredMBOT
{
 public:
  bool operator()(const RuleCubeItemMBOT *p, const RuleCubeItemMBOT *q) const {
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

  const ChartTranslationOptions &GetTranslationOption() const {
       std::cout << "Get translation option NOT IMPLEMENTED for non-mbot queue in RuleCubeMBOT" << std::endl;
  }

  const ChartTranslationOptionMBOT &GetTranslationOptionMBOT() const {
    return m_mbotTransOpt;
  }

 private:

  //Fabienne Braune : TODO : Use boost unordered set instead of stl
  typedef std::set<RuleCubeItemMBOT*, RuleCubeItemPositionOrdererMBOT> ItemSetMBOT;

  //Fabienne Braune : TODO : This can be removed and replaced with the queue in RuleCube.h
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
