/*
 * Misc.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#include "Misc.h"

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

}
}
