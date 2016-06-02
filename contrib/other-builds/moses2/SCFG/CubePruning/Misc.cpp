/*
 * Misc.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#include "Misc.h"
#include "../Manager.h"

namespace Moses2
{

namespace SCFG
{

QueueItem::QueueItem(const SCFG::TargetPhrases &tps)
:tps(tps)
,tpsInd(0)
{
}

void QueueItem::AddHypos(const Moses2::HypothesisColl &hypos)
{
  HyposElement hyposEle(&hypos, 0);
  hyposColl.push_back(hyposEle);

}

void QueueItem::CreateHypo(Manager &mgr)
{
  hypo = SCFG::Hypothesis::Create(mgr.GetPool(), mgr);
  //hypo->Init(mgr, )
}

}
}
