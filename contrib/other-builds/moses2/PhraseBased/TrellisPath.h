/*
 * TrellisPath.h
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include "../TypeDef.h"
#include "ArcLists.h"

namespace Moses2 {

class Scores;
class Hypothesis;
class System;

class TrellishNode
{
  friend std::ostream& operator<<(std::ostream &, const TrellishNode &);

public:
	const ArcList &arcList;
	size_t ind;

	TrellishNode(const ArcList &varcList, size_t vind)
	:arcList(varcList)
	,ind(vind)
	{}
};

class TrellisPath {
public:
	std::vector<const TrellishNode *> nodes;

	TrellisPath();
	TrellisPath(const Hypothesis *hypo, const ArcLists &arcLists);
	virtual ~TrellisPath();

	const Scores &GetScores() const
	{ return *m_scores; }
	SCORE GetFutureScore() const;

	void OutputToStream(std::ostream &out, const System &system) const;

protected:
	const Scores *m_scores;

	void AddNodes(const Hypothesis *hypo, const ArcLists &arcLists);
};

} /* namespace Moses2 */

