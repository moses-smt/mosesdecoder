/*
 * Misc.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#include "Misc.h"
#include "../Manager.h"
#include "../TargetPhrases.h"

namespace Moses2
{

namespace SCFG
{

QueueItem::QueueItem(
    MemPool &pool,
    const SymbolBind &vSymbolBind,
    const SCFG::TargetPhrases &vTPS)
:symbolBind(vSymbolBind)
,tps(vTPS)
,tpInd(0)
{
  m_hyposColl = new (pool.Allocate<HyposColl>()) HyposColl(pool);
}

QueueItem::QueueItem(
    MemPool &pool,
    const SymbolBind &vSymbolBind,
    const SCFG::TargetPhrases &vTPS,
    size_t vTPInd)
:symbolBind(vSymbolBind)
,tps(vTPS)
,tpInd(vTPInd)
,m_hyposColl(NULL)
{
}

void QueueItem::AddHypos(const Moses2::HypothesisColl &hypos)
{
  m_hyposColl->push_back(&hypos);
  hypoIndColl.push_back(0);
}

void QueueItem::CreateHypo(
    SCFG::Manager &mgr,
    const SCFG::InputPath &path,
    const SCFG::SymbolBind &symbolBind)
{
  const SCFG::TargetPhraseImpl &tp = tps[tpInd];

  hypo = SCFG::Hypothesis::Create(mgr.GetPool(), mgr);
  hypo->Init(mgr, path, symbolBind, tp, hypoIndColl);
  hypo->EvaluateWhenApplied();
}

void QueueItem::CreateNext(
    MemPool &pool,
    SCFG::Manager &mgr,
    SCFG::Queue &queue,
    const SCFG::InputPath &path)
{
  if (tpInd + 1 < tps.GetSize()) {
    QueueItem *item = new QueueItem(pool, symbolBind, tps, tpInd + 1);
    item->m_hyposColl = m_hyposColl;
    item->hypoIndColl = hypoIndColl;
    item->CreateHypo(mgr, path, symbolBind);

    queue.push(item);
  }

  assert(m_hyposColl->size() == hypoIndColl.size());
  for (size_t i = 0; i < m_hyposColl->size(); ++i) {
    const Moses2::HypothesisColl &hypos = *(*m_hyposColl)[i];
    size_t hypoInd = hypoIndColl[i];

    if (hypoInd + 1 < hypos.GetSize()) {
      QueueItem *item = new QueueItem(pool, symbolBind, tps, tpInd);

      item->m_hyposColl = m_hyposColl;
      item->hypoIndColl = hypoIndColl;
      item->hypoIndColl[i] = hypoInd + 1;
      item->CreateHypo(mgr, path, symbolBind);

      queue.push(item);
    }
  }

}

}
}
