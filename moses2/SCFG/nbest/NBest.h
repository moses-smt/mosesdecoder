/*
 * NBest.h
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */

#pragma once
#include <queue>
#include <vector>
#include <string>
#include <stdlib.h>
#include "../../Scores.h"
#include "../../ArcLists.h"

namespace Moses2
{
class Scores;
class System;

namespace SCFG
{
class NBest;
class NBests;
class NBestScoreOrderer;
class Manager;
class NBestColl;
class Hypothesis;

/////////////////////////////////////////////////////////////
typedef std::priority_queue<NBest*, std::vector<NBest*>, NBestScoreOrderer> Contenders;

/////////////////////////////////////////////////////////////
class NBest
{
public:
  const ArcList *arcList;
  size_t arcInd;

  typedef std::pair<NBests*, size_t> Child; // key to another NBest
  typedef std::vector<Child> Children;
  Children children;

  NBest(const SCFG::Manager &mgr,
        const ArcList &varcList,
        size_t vind,
        NBestColl &nbestColl);

  NBest(const SCFG::Manager &mgr,
        const NBest &orig,
        size_t childInd,
        NBestColl &nbestColl);


  void CreateDeviants(
    const SCFG::Manager &mgr,
    NBestColl &nbestColl,
    Contenders &contenders) const;

  const Scores &GetScores() const {
    return *m_scores;
  }

  const NBest &GetChild(size_t ind) const;

  const std::string &GetString() const {
    return m_str;
  }

  std::string GetStringExclSentenceMarkers() const {
    std::string ret = m_str.substr(4, m_str.size() - 10);
    return ret;
  }

  std::string Debug(const System &system) const;

protected:
  Scores *m_scores;
  std::string m_str;

  const SCFG::Hypothesis &GetHypo() const;

  void OutputToStream(
    const SCFG::Manager &mgr,
    std::stringstream &strm) const;
};

/////////////////////////////////////////////////////////////
class NBestScoreOrderer
{
public:
  bool operator()(const NBest* a, const NBest* b) const {
    return a->GetScores().GetTotalScore() < b->GetScores().GetTotalScore();
  }
};

}
}

