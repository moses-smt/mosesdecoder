/*
 * Misc.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include "Misc.h"
#include "Manager.h"
#include "TargetPhrases.h"

using namespace std;

namespace Moses2
{

namespace SCFG
{

////////////////////////////////////////////////////////
SeenPosition::SeenPosition(MemPool &pool,
                           const SymbolBind &vSymbolBind,
                           const SCFG::TargetPhrases &vtps,
                           size_t numNT)
  :symbolBind(vSymbolBind)
  ,tps(vtps)
  ,tpInd(0)
  ,hypoIndColl(pool, numNT, 0)
{
}

SeenPosition::SeenPosition(MemPool &pool,
                           const SymbolBind &vSymbolBind,
                           const SCFG::TargetPhrases &vtps,
                           size_t vtpInd,
                           const Vector<size_t> &vhypoIndColl)
  :symbolBind(vSymbolBind)
  ,tps(vtps)
  ,tpInd(vtpInd)
  ,hypoIndColl(pool, vhypoIndColl.size())
{
  for (size_t i = 0; i < hypoIndColl.size(); ++i) {
    hypoIndColl[i] = vhypoIndColl[i];
  }
}

std::string SeenPosition::Debug(const System &system) const
{
  stringstream out;
  out << &tps << " " << tpInd << " ";

  for (size_t i = 0; i < hypoIndColl.size(); ++i) {
    out << hypoIndColl[i] << " ";
  }

  return out.str();
}

bool SeenPosition::operator==(const SeenPosition &compare) const
{
  if (&symbolBind != &compare.symbolBind) {
    return false;
  }

  if (&tps != &compare.tps) {
    return false;
  }

  if (tpInd != compare.tpInd) {
    return false;
  }

  if (hypoIndColl != compare.hypoIndColl) {
    return false;
  }

  return true;
}

size_t SeenPosition::hash() const
{
  size_t ret = (size_t) &symbolBind;
  boost::hash_combine(ret, &tps);
  boost::hash_combine(ret, tpInd);
  boost::hash_combine(ret, hypoIndColl);
  return ret;
}

////////////////////////////////////////////////////////
bool SeenPositions::Add(const SeenPosition *item)
{
  std::pair<Coll::iterator, bool> ret = m_coll.insert(item);
  return ret.second;
}

////////////////////////////////////////////////////////
QueueItem *QueueItem::Create(MemPool &pool, SCFG::Manager &mgr)
{
  //QueueItem *item = new (pool.Allocate<QueueItem>()) QueueItem(pool);
  //return item;

  QueueItemRecycler &queueItemRecycler = mgr.GetQueueItemRecycler();
  QueueItem *ret;
  if (!queueItemRecycler.empty()) {
    // use item from recycle bin
    ret = queueItemRecycler.back();
    queueItemRecycler.pop_back();
  } else {
    // create new item
    ret = new (pool.Allocate<QueueItem>()) QueueItem(pool);
  }

  return ret;

}

QueueItem::QueueItem(MemPool &pool)
  :m_hypoIndColl(NULL)
{

}

void QueueItem::Init(
  MemPool &pool,
  const SymbolBind &vSymbolBind,
  const SCFG::TargetPhrases &vTPS,
  const Vector<size_t> &hypoIndColl)
{
  symbolBind = &vSymbolBind;
  tps = &vTPS;
  tpInd = 0;
  m_hyposColl = new (pool.Allocate<HyposColl>()) HyposColl(pool);
  m_hypoIndColl = &hypoIndColl;
}

void QueueItem::Init(
  MemPool &pool,
  const SymbolBind &vSymbolBind,
  const SCFG::TargetPhrases &vTPS,
  size_t vTPInd,
  const Vector<size_t> &hypoIndColl)
{
  symbolBind = &vSymbolBind;
  tps = &vTPS;
  tpInd = vTPInd;
  m_hyposColl = NULL;
  m_hypoIndColl = &hypoIndColl;
}

void QueueItem::AddHypos(const Moses2::Hypotheses &hypos)
{
  m_hyposColl->push_back(&hypos);
}

void QueueItem::CreateHypo(
  MemPool &systemPool,
  SCFG::Manager &mgr,
  const SCFG::InputPath &path,
  const SCFG::SymbolBind &symbolBind)
{
  const SCFG::TargetPhraseImpl &tp = (*tps)[tpInd];

  hypo = SCFG::Hypothesis::Create(systemPool, mgr);
  hypo->Init(mgr, path, symbolBind, tp, *m_hypoIndColl);
  hypo->EvaluateWhenApplied();
}

void QueueItem::CreateNext(
  MemPool &systemPool,
  MemPool &mgrPool,
  SCFG::Manager &mgr,
  SCFG::Queue &queue,
  SeenPositions &seenPositions,
  const SCFG::InputPath &path)
{
  //cerr << "tpInd=" << tpInd << " " << tps->GetSize() << endl;
  if (tpInd + 1 < tps->GetSize()) {

    const SCFG::TargetPhraseImpl &tp = (*tps)[tpInd + 1];
    SeenPosition *seenItem = new (mgrPool.Allocate<SeenPosition>()) SeenPosition(mgrPool, *symbolBind, *tps, tpInd + 1, *m_hypoIndColl);
    bool unseen = seenPositions.Add(seenItem);

    if (unseen) {
      QueueItem *item = QueueItem::Create(mgrPool, mgr);
      item->Init(mgrPool, *symbolBind, *tps, tpInd + 1, *m_hypoIndColl);
      item->m_hyposColl = m_hyposColl;
      item->CreateHypo(systemPool, mgr, path, *symbolBind);

      queue.push(item);
    }
  }

  assert(m_hyposColl->size() == m_hypoIndColl->size());
  const SCFG::TargetPhraseImpl &tp = (*tps)[tpInd];
  for (size_t i = 0; i < m_hyposColl->size(); ++i) {
    const Moses2::Hypotheses &hypos = *(*m_hyposColl)[i];
    size_t hypoInd = (*m_hypoIndColl)[i] + 1; // increment hypo

    if (hypoInd < hypos.size()) {
      SeenPosition *seenItem = new (mgrPool.Allocate<SeenPosition>()) SeenPosition(mgrPool, *symbolBind, *tps, tpInd, *m_hypoIndColl);
      seenItem->hypoIndColl[i] = hypoInd;
      bool unseen = seenPositions.Add(seenItem);

      if (unseen) {
        QueueItem *item = QueueItem::Create(mgrPool, mgr);
        item->Init(mgrPool, *symbolBind, *tps, tpInd, seenItem->hypoIndColl);

        item->m_hyposColl = m_hyposColl;
        item->CreateHypo(systemPool, mgr, path, *symbolBind);

        queue.push(item);
      }
    }
  }
}

std::string QueueItem::Debug(const System &system) const
{
  stringstream out;
  out << hypo << " " << &(*tps)[tpInd] << "(" << tps << " " << tpInd << ") ";
  for (size_t i = 0; i < m_hypoIndColl->size(); ++i) {
    out << (*m_hypoIndColl)[i] << " ";
  }

  return out.str();
}

}
}
