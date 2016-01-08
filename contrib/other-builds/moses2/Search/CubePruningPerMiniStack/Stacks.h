/*
 * Stacks.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "../CubePruning/Stack.h"
#include "../../Recycler.h"

namespace Moses2
{
class Manager;

namespace NSCubePruningPerMiniStack
{

class Stacks {
	  friend std::ostream& operator<<(std::ostream &, const Stacks &);
public:
	Stacks(const Manager &mgr);
	virtual ~Stacks();

	void Init(size_t numStacks);

	size_t GetSize() const
	{ return m_stacks.size(); }

	const NSCubePruning::Stack &Back() const
    { return *m_stacks.back(); }

	NSCubePruning::Stack &operator[](size_t ind)
    { return *m_stacks[ind]; }

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);
	void Add(const Bitmap &newBitmap, const Range &pathRange);

protected:
	const Manager &m_mgr;
	std::vector<NSCubePruning::Stack*> m_stacks;
};


}

}


