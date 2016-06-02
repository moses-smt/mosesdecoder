/*
 * Misc.h
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include <queue>
#include "../../HypothesisColl.h"
#include "../Hypothesis.h"

namespace Moses2
{

namespace SCFG
{
class TargetPhrases;

///////////////////////////////////////////
class QueueItem
{
public:
  const SCFG::TargetPhrases &tps;
  size_t tpsInd;

  typedef std::pair<const Moses2::HypothesisColl *, size_t> HyposElement;
  std::vector<HyposElement> hyposColl;
    // hypos and ind to the 1 we're using

  SCFG::Hypothesis *hypo;

  QueueItem(const SCFG::TargetPhrases &tps);
  void AddHypos(const Moses2::HypothesisColl &hypos);
  void CreateHypo(Manager &mgr);
};

///////////////////////////////////////////
class QueueItemOrderer
{
public:
  bool operator()(QueueItem* itemA, QueueItem* itemB) const
  {
    HypothesisFutureScoreOrderer orderer;
    return !orderer(itemA->hypo, itemB->hypo);
  }
};

///////////////////////////////////////////
typedef std::priority_queue<QueueItem*,
    std::vector<QueueItem*>,
    QueueItemOrderer> Queue;


}
}



