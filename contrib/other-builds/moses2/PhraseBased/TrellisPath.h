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
class MemPool;
class Hypothesis;
class System;
class TrellisPaths;

class TrellishNode
{
  friend std::ostream& operator<<(std::ostream &, const TrellishNode &);

public:
	const ArcList *arcList;
	size_t ind;

	TrellishNode(const ArcList &varcList, size_t vind)
	:arcList(&varcList)
	,ind(vind)
	{}
};

class TrellisPath {
public:
	std::vector<TrellishNode> nodes;
	int prevEdgeChanged;

	/**< the last node that was wiggled to create this path
	     , or NOT_FOUND if this path is the best trans so consist of only hypos
	  */
	TrellisPath(const Hypothesis *hypo, const ArcLists &arcLists);

	/** create path from another path, deviate at edgeIndex by using arc instead,
	* which may change other hypo back from there
	*/
	TrellisPath(const TrellisPath &origPath,
			size_t edgeIndex,
			const TrellishNode &newNode,
			const ArcLists &arcLists,
			MemPool &pool,
			const System &system);

	virtual ~TrellisPath();

	const Scores &GetScores() const
	{ return *m_scores; }
	SCORE GetFutureScore() const;

	void OutputToStream(std::ostream &out, const System &system) const;

  //! create a set of next best paths by wiggling 1 of the node at a time.
  void CreateDeviantPaths(TrellisPaths &paths,
		  const ArcLists &arcLists,
		  MemPool &pool,
		  const System &system) const;

protected:
  const Scores *m_scores;

  void AddNodes(const Hypothesis *hypo, const ArcLists &arcLists);
  void CalcScores(const Scores &origScores, MemPool &pool, const System &system);
};

} /* namespace Moses2 */

