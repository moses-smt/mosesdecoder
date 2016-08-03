/*
 * TrellisPath.h
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#pragma once
#include "../ArcLists.h"
#include "../TypeDef.h"

namespace Moses2
{
class Scores;
class System;

template<typename T>
class TrellisPaths;

namespace SCFG
{
class Manager;
class Hypothesis;

/////////////////////////////////////////////////////////////////////
class TrellisNode
{
public:
  const ArcList &arcList;
  size_t ind;

  TrellisNode(const ArcLists &arcLists, const SCFG::Hypothesis &hypo);
  TrellisNode(const ArcLists &arcLists, const ArcList &varcList, size_t vind);

  const SCFG::Hypothesis &GetHypothesis() const;
  bool HasMore() const;

  void OutputToStream(std::stringstream &strm) const;

protected:
  std::vector<const TrellisNode*> m_prevNodes;

  void CreateTail(const ArcLists &arcLists, const SCFG::Hypothesis &hypo);
};

/////////////////////////////////////////////////////////////////////
class TrellisPath
{
public:

  TrellisPath(const SCFG::Manager &mgr, const SCFG::Hypothesis &hypo); // create best path
  TrellisPath(const SCFG::Manager &mgr, const SCFG::TrellisPath &origPath, const TrellisNode &nodeToChange); // create original path

  void OutputToStream(std::stringstream &strm);

  const Scores &GetScores() const
  { return *m_scores; }
  Scores &GetScores()
  { return *m_scores; }

  SCORE GetFutureScore() const;

  //! create a set of next best paths by wiggling 1 of the node at a time.
  void CreateDeviantPaths(TrellisPaths<SCFG::TrellisPath> &paths, const SCFG::Manager &mgr) const;

protected:
  Scores *m_scores;
  TrellisNode *m_node;
  TrellisNode *m_prevNodeChanged;

};

} // namespace
}

