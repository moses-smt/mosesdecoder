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
  _HCType m_hypos;

  typedef boost::unordered_map<HyposForCubePruning::HypoCoverage, _HCType> Coll;
  Coll m_coll;

  typedef _HCType::iterator iterator;
  typedef _HCType::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const {
	return m_hypos.begin();
  }
  const_iterator end() const {
	return m_hypos.end();
  }

	StackCubePruning();
	virtual ~StackCubePruning();

	size_t GetInnerSize() const;

	size_t GetSize() const
	{
		//size_t innerSize = GetInnerSize();
		size_t ret = m_hypos.size();
		//assert(innerSize == ret);
		return ret;
	}


	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

	std::vector<const Hypothesis*> GetBestHyposAndPrune(size_t num, Recycler<Hypothesis*> &recycler) const;
	std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
protected:
	StackAdd Add(const Hypothesis *hypo);

	_HCType &GetColl(const HyposForCubePruning::HypoCoverage &key);

};

