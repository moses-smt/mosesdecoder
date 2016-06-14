/*
 * Misc.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include "Misc.h"
#include "../Manager.h"
#include "../TargetPhrases.h"

using namespace std;

namespace Moses2
{

namespace SCFG
{
QueueItem *QueueItem::Create(MemPool &pool, SCFG::Manager &mgr)
{
  QueueItem *item = new (pool.Allocate<QueueItem>()) QueueItem(pool);
  return item;
}

QueueItem::QueueItem(MemPool &pool)
:hypoIndColl(pool)
{

}

void QueueItem::Init(
    MemPool &pool,
    const SymbolBind &vSymbolBind,
    const SCFG::TargetPhrases &vTPS)
{
  symbolBind = &vSymbolBind;
  tps = &vTPS;
  tpInd = 0;
  m_hyposColl = new (pool.Allocate<HyposColl>()) HyposColl(pool);
}

void QueueItem::Init(
    MemPool &pool,
    const SymbolBind &vSymbolBind,
    const SCFG::TargetPhrases &vTPS,
    size_t vTPInd)
{
  symbolBind = &vSymbolBind;
  tps = &vTPS;
  tpInd = vTPInd;
  m_hyposColl = NULL;
}

void QueueItem::AddHypos(const Moses2::HypothesisColl &hypos)
{
  m_hyposColl->push_back(&hypos);
  hypoIndColl.push_back(0);
}

void QueueItem::CreateHypo(
    MemPool &pool,
    SCFG::Manager &mgr,
    const SCFG::InputPath &path,
    const SCFG::SymbolBind &symbolBind)
{
  const SCFG::TargetPhraseImpl &tp = (*tps)[tpInd];

  hypo = SCFG::Hypothesis::Create(pool, mgr);
  hypo->Init(mgr, path, symbolBind, tp, hypoIndColl);
  hypo->EvaluateWhenApplied();
}

void QueueItem::CreateNext(
    MemPool &pool,
    SCFG::Manager &mgr,
    SCFG::Queue &queue,
    SeenPositions &seenPositions,
    const SCFG::InputPath &path)
{
  //cerr << "tpInd=" << tpInd << " " << tps->GetSize() << endl;
  if (tpInd + 1 < tps->GetSize()) {

    const SCFG::TargetPhraseImpl &tp = (*tps)[tpInd + 1];
    SeenPositionItem *seenItem = new (pool.Allocate<SeenPositionItem>()) SeenPositionItem(tps, tpInd, hypoIndColl);
    bool unseen = seenPositions.Add(seenItem);

    if (unseen) {
      QueueItem *item = QueueItem::Create(pool, mgr);
      item->Init(pool, *symbolBind, *tps, tpInd + 1);
      item->m_hyposColl = m_hyposColl;
      item->hypoIndColl = hypoIndColl;
      item->CreateHypo(pool, mgr, path, *symbolBind);

      queue.push(item);
    }
  }

  assert(m_hyposColl->size() == hypoIndColl.size());
  const SCFG::TargetPhraseImpl &tp = (*tps)[tpInd];
  for (size_t i = 0; i < m_hyposColl->size(); ++i) {
    const Moses2::HypothesisColl &hypos = *(*m_hyposColl)[i];
    size_t hypoInd = hypoIndColl[i] + 1;

    if (hypoInd < hypos.GetSize()) {
      SeenPositionItem *seenItem = new (pool.Allocate<SeenPositionItem>()) SeenPositionItem(tps, tpInd, hypoIndColl);
      seenItem->hypoIndColl[i] = hypoInd;
      bool unseen = seenPositions.Add(seenItem);

      if (unseen) {
        QueueItem *item = QueueItem::Create(pool, mgr);
        item->Init(pool, *symbolBind, *tps, tpInd);

        item->m_hyposColl = m_hyposColl;
        item->hypoIndColl = hypoIndColl;
        item->hypoIndColl[i] = hypoInd;
        item->CreateHypo(pool, mgr, path, *symbolBind);

        queue.push(item);
      }
    }
  }
}

void QueueItem::Debug(std::ostream &out, const System &system) const
{
  out << hypo << " " << &(*tps)[tpInd] << "(" << tps << " " << tpInd << ") ";
  for (size_t i = 0; i < hypoIndColl.size(); ++i) {
    out << hypoIndColl[i] << " ";
  }
}

////////////////////////////////////////////////////////
SeenPositionItem::SeenPositionItem(const SCFG::TargetPhrases *vtps, size_t vtpInd, const Vector<size_t> &vhypoIndColl)
:tps(vtps)
,tpInd(vtpInd)
,hypoIndColl(vhypoIndColl.size())
{
  for (size_t i = 0; i < hypoIndColl.size(); ++i) {
    hypoIndColl[i] = vhypoIndColl[i];
  }
}

void SeenPositionItem::Debug(std::ostream &out, const System &system) const
{
  out << tps << " " << tpInd << " ";

  for (size_t i = 0; i < hypoIndColl.size(); ++i) {
    out << hypoIndColl[i] << " ";
  }
}

bool SeenPositionItem::operator==(const SeenPositionItem &compare) const
{
  if (tps != compare.tps) {
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

size_t SeenPositionItem::hash() const
{
  return hash_value(*this);
}

size_t hash_value(const SeenPositionItem& obj)
{
  size_t ret = (size_t) obj.tps;
  boost::hash_combine(ret, obj.tpInd);
  boost::hash_combine(ret, obj.hypoIndColl);
  return ret;
}

////////////////////////////////////////////////////////
bool SeenPositions::Add(const SeenPositionItem *item)
{
  std::pair<Coll::iterator, bool> ret = m_coll.insert(item);
  return ret.second;
}

////////////////////////////////////////////////////////

}
}
