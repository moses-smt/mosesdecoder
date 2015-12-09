/*
 * Stack.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <boost/pool/object_pool.hpp>
#include <boost/unordered_set.hpp>
#include "Hypothesis.h"
#include "../Recycler.h"
#include "../TypeDef.h"
#include "../legacy/Util2.h"

class Stack {
protected:
  typedef boost::unordered_set<const Hypothesis*, UnorderedComparer<Hypothesis>, UnorderedComparer<Hypothesis> > _HCType;
	  _HCType m_hypos;
public:
  typedef _HCType::iterator iterator;
  typedef _HCType::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const {
	return m_hypos.begin();
  }
  const_iterator end() const {
	return m_hypos.end();
  }

	Stack();
	virtual ~Stack();


	size_t GetSize() const
	{ return m_hypos.size(); }

	void Add(const Hypothesis *hypo, boost::object_pool<Hypothesis> &hypoPool);

	std::vector<const Hypothesis*> GetBestHyposAndPrune(size_t num, boost::object_pool<Hypothesis> &hypoPool) const;
	std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
protected:
	StackAdd Add(const Hypothesis *hypo);


};

