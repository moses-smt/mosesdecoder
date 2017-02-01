/*
 * TrellisPath.h
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include "../TypeDef.h"
#include "../ArcLists.h"

namespace Moses2
{

class Scores;
class MemPool;
class Hypothesis;
class System;

template<typename T>
class TrellisPaths;

class TrellisNode
{
public:
  const ArcList *arcList;
  size_t ind;

  TrellisNode(const ArcList &varcList, size_t vind) :
    arcList(&varcList), ind(vind) {
  }

  const HypothesisBase *GetHypo() const {
    return (*arcList)[ind];
  }

  std::string Debug(const System &system) const;

};

class TrellisPath
{
public:
  std::vector<TrellisNode> nodes;
  int prevEdgeChanged;

  /**< the last node that was wiggled to create this path
   , or NOT_FOUND if this path is the best trans so consist of only hypos
   */
  TrellisPath(const Hypothesis *hypo, const ArcLists &arcLists);

  /** create path from another path, deviate at edgeIndex by using arc instead,
   * which may change other hypo back from there
   */
  TrellisPath(const TrellisPath &origPath, size_t edgeIndex,
              const TrellisNode &newNode, const ArcLists &arcLists, MemPool &pool,
              const System &system);

  virtual ~TrellisPath();

  const Scores &GetScores() const {
    return *m_scores;
  }
  SCORE GetFutureScore() const;

  std::string Debug(const System &system) const;

  void OutputToStream(std::ostream &out, const System &system) const;
  std::string OutputTargetPhrase(const System &system) const;

  //! create a set of next best paths by wiggling 1 of the node at a time.
  void CreateDeviantPaths(TrellisPaths<TrellisPath> &paths, const ArcLists &arcLists,
                          MemPool &pool, const System &system) const;

protected:
  const Scores *m_scores;

  void AddNodes(const Hypothesis *hypo, const ArcLists &arcLists);
  void CalcScores(const Scores &origScores, const Scores &origHypoScores,
                  const Scores &newHypoScores, MemPool &pool, const System &system);
};

} /* namespace Moses2 */

