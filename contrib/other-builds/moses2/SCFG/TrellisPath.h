/*
 * TrellisPath.h
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#pragma once
#include "../ArcLists.h"

namespace Moses2
{
class Scores;

namespace SCFG
{
class Manager;
class Hypothesis;

/////////////////////////////////////////////////////////////////////
class TrellisNode
{
public:
  const ArcList *arcList;
  size_t ind;

  TrellisNode(const ArcLists &arcLists, const SCFG::Hypothesis &hypo);
  TrellisNode(const ArcList &varcList, size_t vind) :
      arcList(&varcList), ind(vind)
  {
  }

  const SCFG::Hypothesis &GetHypothesis() const;

  void OutputToStream(std::stringstream &strm) const;

protected:
  std::vector<const TrellisNode*> m_prevNodes;

};

/////////////////////////////////////////////////////////////////////
class TrellisPath
{
public:
  TrellisPath(const SCFG::Manager &mgr, const SCFG::Hypothesis &hypo);
  void OutputToStream(std::stringstream &strm);

  const Scores &GetScores() const
  { return *m_scores; }
  Scores &GetScores()
  { return *m_scores; }

protected:
  Scores *m_scores;
  TrellisNode *m_node;

};

}

}

