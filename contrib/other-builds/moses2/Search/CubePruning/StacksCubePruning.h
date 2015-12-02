/*
 * StacksCubePruning.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "StackCubePruning.h"

class StacksCubePruning {
	  friend std::ostream& operator<<(std::ostream &, const StacksCubePruning &);
public:
	StacksCubePruning();
	virtual ~StacksCubePruning();

	void Init(size_t numStacks);

	size_t GetSize() const
	{ return m_stacks.size(); }

	const StackCubePruning &Back() const
    { return *m_stacks.back(); }

    StackCubePruning &operator[](size_t ind)
    { return *m_stacks[ind]; }

    void Delete(size_t ind) {
    	delete m_stacks[ind];
    	m_stacks[ind] = NULL;
    }

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

protected:
	std::vector<StackCubePruning*> m_stacks;
};

