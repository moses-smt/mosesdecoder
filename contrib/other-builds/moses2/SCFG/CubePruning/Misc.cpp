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
    SeenPositionItem seenItem(tp, hypoIndColl);

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
      SeenPositionItem seenItem(tp, hypoIndColl);
      seenItem.hypoIndColl[i] = hypoInd;

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

////////////////////////////////////////////////////////
SeenPositionItem::SeenPositionItem(const SCFG::TargetPhraseImpl &vtp, const Vector<size_t> &vhypoIndColl)
:tp(&vtp)
,hypoIndColl(vhypoIndColl.size())
{
  for (size_t i = 0; i < hypoIndColl.size(); ++i) {
    hypoIndColl[i] = vhypoIndColl[i];
  }
}

std::ostream& operator<<(std::ostream &out, const SeenPositionItem &obj)
{
  out << obj.tp << " ";

  for (size_t i = 0; i < obj.hypoIndColl.size(); ++i) {
    out << obj.hypoIndColl[i] << " ";
  }
  return out;
}

size_t hash_value(const SeenPositionItem& obj)
{
  size_t ret = boost::hash_value(obj.hypoIndColl);
  boost::hash_combine(ret, (size_t) obj.tp);
  return ret;
}

////////////////////////////////////////////////////////
bool SeenPositions::Add(const SeenPositionItem &item)
{
  std::pair<Coll::iterator, bool> ret = m_coll.insert(item);
  return ret.second;
}

////////////////////////////////////////////////////////

}
}
