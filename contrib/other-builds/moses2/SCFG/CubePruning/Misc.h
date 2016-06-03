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
#include "../../Vector.h"
#include "../Hypothesis.h"

namespace Moses2
{

namespace SCFG
{
class TargetPhrases;
class Queue;

///////////////////////////////////////////
class QueueItem
{
public:
  SCFG::Hypothesis *hypo;

  static QueueItem *Create(MemPool &pool);

  void Init(
      MemPool &pool,
      const SymbolBind &symbolBind,
      const SCFG::TargetPhrases &tps);
  void Init(
      MemPool &pool,
      const SymbolBind &symbolBind,
      const SCFG::TargetPhrases &tps,
      size_t vTPInd);
  void AddHypos(const Moses2::HypothesisColl &hypos);
  void CreateHypo(
      SCFG::Manager &mgr,
      const SCFG::InputPath &path,
      const SCFG::SymbolBind &symbolBind);

  void CreateNext(
      MemPool &pool,
      SCFG::Manager &mgr,
      SCFG::Queue &queue,
      const SCFG::InputPath &path);

protected:
  typedef Vector<const Moses2::HypothesisColl *> HyposColl;
  HyposColl *m_hyposColl;

  const SymbolBind *symbolBind;
  const SCFG::TargetPhrases *tps;
  size_t tpInd;

  Vector<size_t> hypoIndColl;
    // hypos and ind to the 1 we're using

  QueueItem(MemPool &pool);

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
class Queue : public std::priority_queue<QueueItem*,
    std::vector<QueueItem*>,
    QueueItemOrderer>
{

};


}
}



