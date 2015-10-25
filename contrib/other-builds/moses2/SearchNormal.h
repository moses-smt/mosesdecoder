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
class InputPath;
class TargetPhrases;

class SearchNormal {
public:
	SearchNormal(Manager &mgr, std::vector<Stack> &stacks);
	virtual ~SearchNormal();

	void Decode(size_t stackInd);

protected:
	Manager &m_mgr;
	std::vector<Stack> &m_stacks;

	void Extend(const Hypothesis &hypo);
	void Extend(const Hypothesis &hypo, const InputPath &path);
	void Extend(const Hypothesis &hypo, const TargetPhrases &tps);
};

#endif /* SEARCHNORMAL_H_ */
