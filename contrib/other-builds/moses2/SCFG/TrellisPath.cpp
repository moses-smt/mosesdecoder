/*
 * TrellisPath.cpp
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#include "TrellisPath.h"
#include "Hypothesis.h"
#include "Manager.h"
#include "TargetPhraseImpl.h"

using namespace std;

namespace Moses2
{

namespace SCFG
{
TrellisNode::TrellisNode(const ArcLists &arcLists, const SCFG::Hypothesis &hypo)
{
  arcList = &arcLists.GetArcList(&hypo);
  ind = 0;

  const Vector<const Hypothesis*> &prevHypos = hypo.GetPrevHypos();
  m_prevNodes.resize(prevHypos.size(), NULL);

  for (size_t i = 0; i < hypo.GetPrevHypos().size(); ++i) {
    const SCFG::Hypothesis &prevHypo = *prevHypos[i];
    TrellisNode *prevNode = new TrellisNode(arcLists, prevHypo);
    m_prevNodes[i] = prevNode;
  }
}

void TrellisNode::OutputToStream(std::stringstream &strm) const
{
  const SCFG::Hypothesis &hypo = *static_cast<const SCFG::Hypothesis*>((*arcList)[ind]);
  const SCFG::TargetPhraseImpl &tp = hypo.GetTargetPhrase();
  //cerr << "tp=" << tp.Debug(m_mgr->system) << endl;

  for (size_t pos = 0; pos < tp.GetSize(); ++pos) {
	const SCFG::Word &word = tp[pos];
	//cerr << "word " << pos << "=" << word << endl;
	if (word.isNonTerminal) {
	  //cerr << "is nt" << endl;
	  // non-term. fill out with prev hypo
	  size_t nonTermInd = tp.GetAlignNonTerm().GetNonTermIndexMap()[pos];
	  const TrellisNode *prevNode = m_prevNodes[nonTermInd];
	  cerr << "prevNode=" << prevNode << endl;

	  prevNode->OutputToStream(strm);
	}
	else {
	  //cerr << "not nt" << endl;
	  word.OutputToStream(strm);
	  strm << " ";
	}
  }
}


/////////////////////////////////////////////////////////////////////

TrellisPath::TrellisPath(const SCFG::Manager &mgr, const SCFG::Hypothesis &hypo)
{
  // 1st
  m_scores = &hypo.GetScores();
  m_node = new TrellisNode(mgr.arcLists, hypo);
}

void TrellisPath::OutputToStream(std::stringstream &strm)
{
	m_node->OutputToStream(strm);
}

}
}



