/*
 * KBestExtractor.cpp
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */

#include "KBestExtractor.h"
#include "Manager.h"
#include "Hypothesis.h"
#include "Stacks.h"
#include "Stack.h"
#include "TrellisPath.h"

namespace Moses2
{
namespace SCFG
{

KBestExtractor::KBestExtractor(const SCFG::Manager &mgr)
:m_mgr(mgr)
{
  ArcLists &arcLists = mgr.arcLists;
  const Stack &lastStack = mgr.GetStacks().GetLastStack();
  const Hypothesis *bestHypo = lastStack.GetBestHypo(mgr, arcLists);

  if (bestHypo) {
    TrellisPath *path = new TrellisPath(*bestHypo);
  }
}

KBestExtractor::~KBestExtractor()
{
  // TODO Auto-generated destructor stub
}

}
} /* namespace Moses2 */
