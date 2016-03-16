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


class TrellishNode
{
public:
	ArcList &arcList;
	size_t ind;

	TrellishNode(ArcList &varcList, size_t vind)
	:arcList(varcList)
	,ind(vind)
	{}
};

class TrellisPath {
public:
	std::vector<const TrellishNode *> nodes;

	TrellisPath();
	virtual ~TrellisPath();

	SCORE GetFutureScore() const;
protected:
};

} /* namespace Moses2 */

