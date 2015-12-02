/*
 * StacksCubePruning.cpp
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#include "StacksCubePruning.h"

using namespace std;

StacksCubePruning::StacksCubePruning() {
	// TODO Auto-generated constructor stub

}

StacksCubePruning::~StacksCubePruning() {
	for (size_t i = 0; i < m_stacks.size(); ++i) {
		delete m_stacks[i];
	}
}

void StacksCubePruning::Init(size_t numStacks)
{
	m_stacks.resize(numStacks);
	for (size_t i = 0; i < m_stacks.size(); ++i) {
		m_stacks[i] = new StackCubePruning();
	}
}


std::ostream& operator<<(std::ostream &out, const StacksCubePruning &obj)
{
  for (size_t i = 0; i < obj.GetSize(); ++i) {
	  const StackCubePruning *stack = obj.m_stacks[i];
	  if (stack) {
		  out << stack->GetHypoSize() << " ";
	  }
	  else {
		  out << "N ";
	  }
  }

  return out;
}

void StacksCubePruning::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
	size_t numWordsCovered = hypo->GetBitmap().GetNumWordsCovered();
	//cerr << "numWordsCovered=" << numWordsCovered << endl;
	StackCubePruning &stack = *m_stacks[numWordsCovered];
	stack.Add(hypo, hypoRecycle);

}

