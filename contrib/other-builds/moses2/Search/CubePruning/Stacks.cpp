/*
 * Stacks.cpp
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#include "Stacks.h"
#include "../../System.h"
#include "../Manager.h"

using namespace std;

namespace Moses2
{

namespace NSCubePruning
{

Stacks::Stacks(const Manager &mgr)
:m_mgr(mgr)
{
}

Stacks::~Stacks()
{
	Recycler<NSCubePruning::Stack*> &recycler = m_mgr.system.GetStackRecycler();
	for (size_t i = 0; i < m_stacks.size(); ++i) {
		recycler.Add(m_stacks[i]);
	}
}

void Stacks::Init(size_t numStacks)
{
	Recycler<NSCubePruning::Stack*> &recycler = m_mgr.system.GetStackRecycler();

	m_stacks.resize(numStacks);
	for (size_t i = 0; i < m_stacks.size(); ++i) {
		if (recycler.IsEmpty()) {
			m_stacks[i] = new Stack();
		}
		else {
			Stack *stack = recycler.Get();
			recycler.Pop();
			stack->Clear();

			m_stacks[i] = stack;
		}
	}
}


std::ostream& operator<<(std::ostream &out, const Stacks &obj)
{
  for (size_t i = 0; i < obj.GetSize(); ++i) {
	  const Stack &stack = *obj.m_stacks[i];
	  out << stack.GetHypoSize() << " ";
  }

  return out;
}

void Stacks::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
	size_t numWordsCovered = hypo->GetBitmap().GetNumWordsCovered();
	//cerr << "numWordsCovered=" << numWordsCovered << endl;
	Stack &stack = *m_stacks[numWordsCovered];
	stack.Add(hypo, hypoRecycle);

}

}

}


