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
#include "../TrellisPaths.h"

using namespace std;

namespace Moses2
{

namespace SCFG
{
TrellisNode::TrellisNode(const ArcLists &arcLists, const SCFG::Hypothesis &hypo)
:arcList(arcLists.GetArcList(&hypo))
{
  UTIL_THROW_IF2(arcList.size() == 0, "Empty arclist");
  ind = 0;

  CreateTail(arcLists, hypo);
}

TrellisNode::TrellisNode(const ArcLists &arcLists, const ArcList &varcList, size_t vind)
:arcList(varcList)
 ,ind(vind)
{
  UTIL_THROW_IF2(vind >= arcList.size(), "arclist out of bound" << ind << " >= " << arcList.size());
  const SCFG::Hypothesis &hypo = arcList[ind]->Cast<SCFG::Hypothesis>();
  CreateTail(arcLists, hypo);
}

void TrellisNode::CreateTail(const ArcLists &arcLists, const SCFG::Hypothesis &hypo)
{
  const Vector<const Hypothesis*> &prevHypos = hypo.GetPrevHypos();
  m_prevNodes.resize(prevHypos.size(), NULL);

  for (size_t i = 0; i < hypo.GetPrevHypos().size(); ++i) {
	const SCFG::Hypothesis &prevHypo = *prevHypos[i];
	TrellisNode *prevNode = new TrellisNode(arcLists, prevHypo);
	m_prevNodes[i] = prevNode;
  }
}

const SCFG::Hypothesis &TrellisNode::GetHypothesis() const
{
  UTIL_THROW_IF2(ind >= arcList.size(), "Arcs requested out of bound. " << arcList.size() << "<" << ind);
  const SCFG::Hypothesis &hypo = arcList[ind]->Cast<SCFG::Hypothesis>();
  return hypo;
}

void TrellisNode::OutputToStream(std::stringstream &strm) const
{
  const SCFG::Hypothesis &hypo = GetHypothesis();
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

	  prevNode->OutputToStream(strm);
	}
	else {
	  //cerr << "not nt" << endl;
	  word.OutputToStream(strm);
	  strm << " ";
	}
  }
}

bool TrellisNode::HasMore() const
{
	bool ret = arcList.size() > (ind + 1);
	return ret;
}

/////////////////////////////////////////////////////////////////////

TrellisPath::TrellisPath(const SCFG::Manager &mgr, const SCFG::Hypothesis &hypo)
{
  MemPool &pool = mgr.GetPool();

  // 1st
  m_scores =   m_scores = new (pool.Allocate<Scores>())
		  Scores(mgr.system,  pool, mgr.system.featureFunctions.GetNumScores(), hypo.GetScores());
  m_node = new TrellisNode(mgr.arcLists, hypo);
  m_prevNodeChanged = m_node;
}

TrellisPath::TrellisPath(const SCFG::Manager &mgr, const SCFG::TrellisPath &origPath, const TrellisNode &nodeToChange)
{
	if (origPath.m_node == &nodeToChange) {
		m_node = new TrellisNode(mgr.arcLists, nodeToChange.arcList, nodeToChange.ind + 1);
	}
}

void TrellisPath::OutputToStream(std::stringstream &strm)
{
	m_node->OutputToStream(strm);
}

SCORE TrellisPath::GetFutureScore() const
{
  return m_scores->GetTotalScore();
}

//! create a set of next best paths by wiggling 1 of the node at a time.
void TrellisPath::CreateDeviantPaths(TrellisPaths<SCFG::TrellisPath> &paths, const SCFG::Manager &mgr) const
{
	if (m_prevNodeChanged->HasMore()) {
		cerr << "BEGIN deviantPath" << endl;
		SCFG::TrellisPath *deviantPath = new TrellisPath(mgr, *this, *m_prevNodeChanged);
		cerr << "END deviantPath" << endl;
		paths.Add(deviantPath);
		cerr << "ADDED deviantPath" << endl;
	}
}

} // namespace
}



