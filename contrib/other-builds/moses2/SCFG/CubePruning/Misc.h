/*
 * Misc.h
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include <queue>
#include <boost/unordered_set.hpp>
#include "../../HypothesisColl.h"
#include "../../Vector.h"
#include "../Hypothesis.h"

namespace Moses2
{

namespace SCFG
{
class TargetPhrases;
class Queue;
class SeenPositions;

///////////////////////////////////////////
class QueueItem
{
public:
  SCFG::Hypothesis *hypo;

  static QueueItem *Create(MemPool &pool, SCFG::Manager &mgr);

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
      MemPool &pool,
      SCFG::Manager &mgr,
      const SCFG::InputPath &path,
      const SCFG::SymbolBind &symbolBind);

  void CreateNext(
      MemPool &pool,
      SCFG::Manager &mgr,
      SCFG::Queue &queue,
      SeenPositions &seenPositions,
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

typedef std::deque<QueueItem*> QueueItemRecycler;

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

///////////////////////////////////////////
class SeenPositionItem
{
  friend std::ostream& operator<<(std::ostream &out, const SeenPositionItem &obj);

public:
  const SCFG::TargetPhraseImpl *tp;
  std::vector<size_t> hypoIndColl;

  SeenPositionItem(const SCFG::TargetPhraseImpl &vtp, const Vector<size_t> &vhypoIndColl);

  virtual bool operator==(const SeenPositionItem &compare) const
  {
    bool ret = (tp == compare.tp) && (hypoIndColl == compare.hypoIndColl);
    return ret;
  }

};

size_t hash_value(const SeenPositionItem& obj);

///////////////////////////////////////////

class SeenPositions
{
public:
  bool Add(const SeenPositionItem &item);

  void clear()
  { m_coll.clear(); }


protected:
  typedef boost::unordered_set<SeenPositionItem> Coll;
  Coll m_coll;
};

}
}



