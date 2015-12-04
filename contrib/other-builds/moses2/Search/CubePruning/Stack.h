/*
 * Stack.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "../Hypothesis.h"
#include "Misc.h"
#include "../../Recycler.h"
#include "../../TypeDef.h"
#include "../../legacy/Util2.h"

class Manager;

namespace NSCubePruning
{

class HypothesisSet
{
public:
	typedef boost::unordered_set<const Hypothesis*,
			  UnorderedComparer<Hypothesis>, UnorderedComparer<Hypothesis>
			> _HCType;

	_HCType &GetColl()
	{ return m_coll; }

	const _HCType &GetColl() const
	{ return m_coll; }

	CubeEdge::Hypotheses &GetSortedHypos(const Manager &mgr) const;

protected:
	_HCType m_coll;
	mutable CubeEdge::Hypotheses m_sortedHypos;

	void SortAndPruneHypos(const Manager &mgr) const;

};

/////////////////////////////////////////////
class Stack {
protected:


public:
  typedef std::pair<const Bitmap*, size_t> HypoCoverage;
		  // bitmap and current endPos of hypos

  typedef boost::unordered_map<HypoCoverage, HypothesisSet> Coll;


	Stack();
	virtual ~Stack();

	size_t GetHypoSize() const;

	Coll &GetColl()
	{ return m_coll; }

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

	std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
protected:
	Coll m_coll;

	StackAdd Add(const Hypothesis *hypo);

	HypothesisSet &GetHypothesisSet(const HypoCoverage &key);

};

}


