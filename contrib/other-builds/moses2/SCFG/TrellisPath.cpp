/*
 * TrellisPath.cpp
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#include "TrellisPath.h"
#include "Hypothesis.h"
#include "Manager.h"

namespace Moses2
{

namespace SCFG
{
TrellisNode::TrellisNode(const ArcLists &arcLists, const SCFG::Hypothesis &hypo)
{
  arcList = arcLists.GetArcList(&hypo);
  ind = 0;

  const Vector<const Hypothesis*> &prevHypos = hypo.GetPrevHypos();
  m_prevNodes.resize(prevHypos.size());

  for (size_t i = 0; i < hypo.GetPrevHypos().size(); ++i) {
    const SCFG::Hypothesis &prevHypo = *prevHypos[i];
    TrellisNode *prevNode = new TrellisNode(arcLists, prevHypo);
    m_prevNodes[i] = prevNode;
  }
}

/////////////////////////////////////////////////////////////////////

TrellisPath::TrellisPath(const SCFG::Manager &mgr, const SCFG::Hypothesis &hypo)
{
  // 1st
  m_scores = &hypo.GetScores();
  m_node = new TrellisNode(mgr.arcLists, hypo);
}

void TrellisPath::Output(std::stringstream &strm)
{

}

}
}



