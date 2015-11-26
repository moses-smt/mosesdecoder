/*
 * Stacks.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#ifndef SEARCH_STACKS_H_
#define SEARCH_STACKS_H_

#include <vector>
#include "Stack.h"

class Stacks {
	  friend std::ostream& operator<<(std::ostream &, const Stacks &);
public:
	Stacks();
	virtual ~Stacks();

	void Init(size_t numStacks);

	size_t GetSize() const
	{ return m_stacks.size(); }

    Stack &Back()
    { return *m_stacks.back(); }

    Stack &operator[](size_t ind)
    { return *m_stacks[ind]; }

    void Delete(size_t ind) {
    	delete m_stacks[ind];
    	m_stacks[ind] = NULL;
    }

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

protected:
	std::vector<Stack*> m_stacks;
};

#endif /* SEARCH_STACKS_H_ */
