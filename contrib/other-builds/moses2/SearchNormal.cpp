/*
 * SearchNormal.cpp
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#include "SearchNormal.h"
#include "Stack.h"

SearchNormal::SearchNormal(const Manager &mgr, std::vector<Stack> &stacks)
:m_mgr(mgr)
,m_stacks(stacks)
{
	// TODO Auto-generated constructor stub

}

SearchNormal::~SearchNormal() {
	// TODO Auto-generated destructor stub
}

void SearchNormal::Decode(size_t stackInd)
{
	Stack &stack = m_stacks[stackInd];

	Stack::const_iterator iter;
	for (iter = stack.begin(); iter != stack.end(); ++iter) {
		const Hypothesis &hypo = **iter;
		Extend(hypo);
	}
}

void SearchNormal::Extend(const Hypothesis &hypo)
{

}


