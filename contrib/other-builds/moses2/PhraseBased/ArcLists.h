/*
 * ArcList.h
 *
 *  Created on: 26 Oct 2015
 *      Author: hieu
 */

#ifndef ARCLISTS_H_
#define ARCLISTS_H_
#include <vector>
#include <boost/unordered_map.hpp>

namespace Moses2
{

class HypothesisBase;

typedef std::vector<const HypothesisBase*> ArcList;

class ArcLists {
public:
	ArcLists();
	virtual ~ArcLists();

	void AddArc(bool added, const HypothesisBase *currHypo,const HypothesisBase *otherHypo);

protected:
	typedef boost::unordered_map<const HypothesisBase*, ArcList*> Coll;
	Coll m_coll;

	ArcList *GetArcList(const HypothesisBase *hypo);
	ArcList *GetAndDetachArcList(const HypothesisBase *hypo);

};

}

#endif /* ARCLISTS_H_ */
