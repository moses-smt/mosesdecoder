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
  size_t tpInd;

  std::vector<const Moses2::HypothesisColl *> hyposColl;
  std::vector<size_t> hypoIndColl;
    // hypos and ind to the 1 we're using

  SCFG::Hypothesis *hypo;

  QueueItem(const SCFG::TargetPhrases &tps);
  void AddHypos(const Moses2::HypothesisColl &hypos);
  void CreateHypo(SCFG::Manager &mgr,
      const SCFG::InputPath &path,
      const SCFG::SymbolBind &symbolBind);
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



