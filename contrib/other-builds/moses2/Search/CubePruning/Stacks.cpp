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
}

void Stacks::Init(size_t numStacks)
{
	m_stacks.resize(numStacks);
	for (size_t i = 0; i < m_stacks.size(); ++i) {
	  m_stacks[i] = new (m_mgr.GetPool().Allocate<Stack>()) Stack(m_mgr);
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


