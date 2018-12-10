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
#include "../HypothesisColl.h"
#include "../Vector.h"
#include "Hypothesis.h"

namespace Moses2
{

namespace SCFG
{
class SymbolBind;
class TargetPhrases;
class Queue;

///////////////////////////////////////////
class SeenPosition
{
public:
  const SymbolBind &symbolBind;
  const SCFG::TargetPhrases &tps;
  size_t tpInd;
  Vector<size_t> hypoIndColl;

  SeenPosition(MemPool &pool,
               const SymbolBind &vSymbolBind,
               const SCFG::TargetPhrases &vtps,
               size_t numNT);
  SeenPosition(MemPool &pool,
               const SymbolBind &vSymbolBind,
               const SCFG::TargetPhrases &vtps,
               size_t vtpInd,
               const Vector<size_t> &vhypoIndColl);

  bool operator==(const SeenPosition &compare) const;
  size_t hash() const;

  std::string Debug(const System &system) const;

};

///////////////////////////////////////////

class SeenPositions
{
public:
  bool Add(const SeenPosition *item);

  void clear() {
    m_coll.clear();
  }


protected:
  typedef boost::unordered_set<const SeenPosition*,
		  UnorderedComparer<SeenPosition>, UnorderedComparer<SeenPosition> > Coll;
  Coll m_coll;
};

///////////////////////////////////////////
class QueueItem
{
public:
  SCFG::Hypothesis *hypo;

  static QueueItem *Create(MemPool &pool, SCFG::Manager &mgr);

  void Init(
    MemPool &pool,
    const SymbolBind &symbolBind,
    const SCFG::TargetPhrases &tps,
    const Vector<size_t> &hypoIndColl);
  void Init(
    MemPool &pool,
    const SymbolBind &symbolBind,
    const SCFG::TargetPhrases &tps,
    size_t vTPInd,
    const Vector<size_t> &hypoIndColl);
  void AddHypos(const Moses2::Hypotheses &hypos);
  void CreateHypo(
    MemPool &systemPool,
    SCFG::Manager &mgr,
    const SCFG::InputPath &path,
    const SCFG::SymbolBind &symbolBind);

  void CreateNext(
    MemPool &systemPool,
    MemPool &mgrPool,
    SCFG::Manager &mgr,
    SCFG::Queue &queue,
    SeenPositions &seenPositions,
    const SCFG::InputPath &path);

  std::string Debug(const System &system) const;

protected:
  typedef Vector<const Moses2::Hypotheses *> HyposColl;
  HyposColl *m_hyposColl;

  const SymbolBind *symbolBind;
  const SCFG::TargetPhrases *tps;
  size_t tpInd;

  const Vector<size_t> *m_hypoIndColl; // pointer to variable in seen position
  // hypos and ind to the 1 we're using

  QueueItem(MemPool &pool);

};

///////////////////////////////////////////

typedef std::deque<QueueItem*> QueueItemRecycler;

///////////////////////////////////////////
class QueueItemOrderer
{
public:
  bool operator()(QueueItem* itemA, QueueItem* itemB) const {
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



