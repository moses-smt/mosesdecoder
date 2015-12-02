/*
 * StackCubePruning.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "Hypothesis.h"
#include "CubePruning.h"
#include "../Recycler.h"
#include "../TypeDef.h"
#include "../legacy/Util2.h"

class StackCubePruning {
protected:


public:
  typedef boost::unordered_set<const Hypothesis*, UnorderedComparer<Hypothesis>, UnorderedComparer<Hypothesis>, MemPoolAllocator<const Hypothesis*> > _HCType;
  typedef boost::unordered_map<HyposForCubePruning::HypoCoverage, _HCType> Coll;


	StackCubePruning();
	virtual ~StackCubePruning();

	size_t GetHypoSize() const;

	const Coll &GetColl() const
	{ return m_coll; }

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

	std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
protected:
	  Coll m_coll;

	StackAdd Add(const Hypothesis *hypo);

	_HCType &GetColl(const HyposForCubePruning::HypoCoverage &key);

};

