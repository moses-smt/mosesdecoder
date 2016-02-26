/*
 * Stack.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <boost/unordered_set.hpp>
#include <deque>
#include "../Hypothesis.h"
#include "../../TypeDef.h"
#include "../../Recycler.h"
#include "../../legacy/Util2.h"

namespace Moses2
{

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

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

	std::vector<const Hypothesis*> GetBestHyposAndPrune(size_t num, Recycler<Hypothesis*> &recycler) const;
	std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
protected:
	StackAdd Add(const Hypothesis *hypo);


};

}

