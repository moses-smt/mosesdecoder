/*
 * TrellisPath.h
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#pragma once
#include "../ArcLists.h"
#include "../TypeDef.h"
#include "../Vector.h"

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
  typedef std::vector<const TrellisNode*> Children;

  const ArcList &arcList;
  size_t ind;

  TrellisNode(const SCFG::Manager &mgr, const ArcLists &arcLists, const SCFG::Hypothesis &hypo);
  TrellisNode(const SCFG::Manager &mgr, const ArcLists &arcLists, const ArcList &varcList, size_t vind);
  TrellisNode(const SCFG::Manager &mgr, const ArcLists &arcLists, const TrellisNode &orig, const TrellisNode &nodeToChange);

  virtual ~TrellisNode();

  const SCFG::Hypothesis &GetHypothesis() const;
  bool HasMore() const;
  const Children &GetChildren() const
  { return m_prevNodes; }

  void OutputToStream(const System &system, std::stringstream &strm) const;

protected:
  Children m_prevNodes;

  void CreateTail(const SCFG::Manager &mgr, const ArcLists &arcLists, const SCFG::Hypothesis &hypo);
};

/////////////////////////////////////////////////////////////////////
class TrellisPath
{
public:

  TrellisPath(const SCFG::Manager &mgr, const SCFG::Hypothesis &hypo); // create best path
  TrellisPath(const SCFG::Manager &mgr, const SCFG::TrellisPath &origPath, const TrellisNode &nodeToChange); // create original path
  ~TrellisPath();

  const std::string &Output() const
  { return m_out; }

  const Scores &GetScores() const
  { return *m_scores; }
  Scores &GetScores()
  { return *m_scores; }

  SCORE GetFutureScore() const;

  //! create a set of next best paths by wiggling 1 of the node at a time.
  void CreateDeviantPaths(TrellisPaths<SCFG::TrellisPath> &paths, const SCFG::Manager &mgr) const;

  std::string Debug(const System &system) const;

protected:
  Scores *m_scores;
  TrellisNode *m_node;
  TrellisNode *m_prevNodeChanged;
  std::string m_out;

  void CreateDeviantPaths(
      TrellisPaths<SCFG::TrellisPath> &paths,
      const SCFG::Manager &mgr,
      const TrellisNode &parentNode) const;

  void ComputeStr(const System &system);

};

} // namespace
}

