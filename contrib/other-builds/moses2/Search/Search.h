/*
 * Search.h
 *
 *  Created on: 16 Nov 2015
 *      Author: hieu
 */

#ifndef SEARCH_SEARCH_H_
#define SEARCH_SEARCH_H_

#include <stddef.h>

class Manager;
class Stack;
class Stacks;
class Hypothesis;

class Search {
public:
	Search(Manager &mgr, Stacks &stacks);
	virtual ~Search();

	virtual void Decode(size_t stackInd) = 0;

	virtual const Hypothesis *GetBestHypothesis() const = 0;

protected:
	Manager &m_mgr;
	Stacks &m_stacks;
	//ArcLists m_arcLists;

};

#endif /* SEARCH_SEARCH_H_ */
