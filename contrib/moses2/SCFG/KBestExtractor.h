/*
 * KBestExtractor.h
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include <sstream>
#include <boost/unordered_map.hpp>
#include "../ArcLists.h"

namespace Moses2
{
class Scores;

namespace SCFG
{
class Manager;
class Hypothesis;

class NBestCandidate
{
public:
	const ArcList *arcList;
	size_t ind;

	typedef std::pair<const ArcList *, size_t> Child;
	typedef std::vector<Child> Children;
	Children children;

	NBestCandidate(const SCFG::Manager &mgr, const ArcList &varcList, size_t vind);

	const Scores &GetScores() const
	{ return *m_scores; }

protected:
	Scores *m_scores;
};

/////////////////////////////////////////////////////////////
typedef std::vector<NBestCandidate> NBestCandidates;

/////////////////////////////////////////////////////////////
class NBestColl
{
public:
	void Add(const SCFG::Manager &mgr, const ArcList &arcList);
	const NBestCandidates &GetNBestCandidates(const ArcList &arcList);

protected:
	typedef boost::unordered_map<const ArcList*, NBestCandidates> Coll;
	Coll m_candidates;
};

/////////////////////////////////////////////////////////////
class KBestExtractor
{
public:
  KBestExtractor(const SCFG::Manager &mgr);
  virtual ~KBestExtractor();

  void OutputToStream(std::stringstream &strm);
protected:
  const SCFG::Manager &m_mgr;
  NBestColl m_nbestColl;
};

}
} /* namespace Moses2 */
