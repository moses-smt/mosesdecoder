/*
 * Stacks.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "Stack.h"

namespace Moses2
{
class Manager;

namespace NSCubePruning
{

class Stacks {
	  friend std::ostream& operator<<(std::ostream &, const Stacks &);
public:
	Stacks(const Manager &mgr);
	virtual ~Stacks();

	void Init(size_t numStacks);

	size_t GetSize() const
	{ return m_stacks.size(); }

	const Stack &Back() const
    { return *m_stacks.back(); }

    Stack &operator[](size_t ind)
    { return *m_stacks[ind]; }

	void Add(const Hypothesis *hypo, StackAdd &added);

protected:
	const Manager &m_mgr;
	std::vector<Stack*> m_stacks;
};


}

}


