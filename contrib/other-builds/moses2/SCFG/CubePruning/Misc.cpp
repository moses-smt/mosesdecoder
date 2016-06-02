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

QueueItem::QueueItem(const SCFG::TargetPhrases &tps, size_t vTPInd)
:tps(tps)
,tpInd(vTPInd)
{
}

void QueueItem::AddHypos(const Moses2::HypothesisColl &hypos)
{
  hyposColl.push_back(&hypos);
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
}

void QueueItem::CreateNext(
    SCFG::Manager &mgr,
    SCFG::Queue &queue,
    const SCFG::InputPath &path)
{
  if (tpInd + 1 < tps.GetSize()) {
    QueueItem *item = new QueueItem(tps, tpInd + 1);
    item->hyposColl = hyposColl;
    item->hypoIndColl = hypoIndColl;
    item->CreateHypo(mgr, path, hypo->GetSymbolBind());

    queue.push(item);
  }

  assert(hyposColl.size() == hypoIndColl.size());
  for (size_t i = 0; i < hyposColl.size(); ++i) {
    const Moses2::HypothesisColl &hypos = *hyposColl[i];
    size_t hypoInd = hypoIndColl[i];

    if (hypoInd + 1 < hypos.GetSize()) {
      QueueItem *item = new QueueItem(tps, tpInd);

      item->hyposColl = hyposColl;
      item->hypoIndColl = hypoIndColl;

      item->hypoIndColl[i] = hypoInd + 1;

      item->CreateHypo(mgr, path, hypo->GetSymbolBind());

      queue.push(item);
    }
  }

}

}
}
