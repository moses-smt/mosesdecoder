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

QueueItem::QueueItem(const SCFG::TargetPhrases &tps)
:tps(tps)
,tpInd(0)
{
}

void QueueItem::AddHypos(const Moses2::HypothesisColl &hypos)
{
  hyposColl.push_back(&hypos);
  hypoIndColl.push_back(0);
}

void QueueItem::CreateHypo(SCFG::Manager &mgr,
    const SCFG::InputPath &path,
    const SCFG::SymbolBind &symbolBind)
{
  const SCFG::TargetPhraseImpl &tp = tps[tpInd];

  hypo = SCFG::Hypothesis::Create(mgr.GetPool(), mgr);
  hypo->Init(mgr, path, symbolBind, tp, hypoIndColl);
}

}
}
