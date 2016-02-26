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

class Hypothesis;

typedef std::vector<const Hypothesis*> ArcList;

class ArcLists {
public:
	ArcLists();
	virtual ~ArcLists();

	void AddArc(bool added, const Hypothesis *currHypo,const Hypothesis *otherHypo);

protected:
	typedef boost::unordered_map<const Hypothesis*, ArcList*> Coll;
	Coll m_coll;

	ArcList *GetArcList(const Hypothesis *hypo);
	ArcList *GetAndDetachArcList(const Hypothesis *hypo);

};

}

#endif /* ARCLISTS_H_ */
