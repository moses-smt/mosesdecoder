/*
 * Stacks.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <boost/pool/object_pool.hpp>
#include <vector>
#include "Stack.h"

namespace NSCubePruning
{

class Stacks {
	  friend std::ostream& operator<<(std::ostream &, const Stacks &);
public:
	Stacks();
	virtual ~Stacks();

	void Init(size_t numStacks);

	size_t GetSize() const
	{ return m_stacks.size(); }

	const Stack &Back() const
    { return m_stacks.back(); }

    Stack &operator[](size_t ind)
    { return m_stacks[ind]; }

	void Add(const Hypothesis *hypo, boost::object_pool<Hypothesis> &hypoPool);

protected:
	std::vector<Stack> m_stacks;
};


}


