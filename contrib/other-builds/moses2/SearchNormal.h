/*
 * SearchNormal.h
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#ifndef SEARCHNORMAL_H_
#define SEARCHNORMAL_H_
#include <vector>

class Manager;
class Stack;
class Hypothesis;

class SearchNormal {
public:
	SearchNormal(const Manager &mgr, std::vector<Stack> &stacks);
	virtual ~SearchNormal();

	void Decode(size_t stackInd);

protected:
	const Manager &m_mgr;
	std::vector<Stack> &m_stacks;

	void Extend(const Hypothesis &hypo);
};

#endif /* SEARCHNORMAL_H_ */
