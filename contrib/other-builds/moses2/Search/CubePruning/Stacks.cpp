/*
 * Stacks.cpp
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#include "Stacks.h"

using namespace std;

namespace NSCubePruning
{

Stacks::Stacks() {
	// TODO Auto-generated constructor stub

}

Stacks::~Stacks() {
}

void Stacks::Init(size_t numStacks)
{
	m_stacks.resize(numStacks);
}


std::ostream& operator<<(std::ostream &out, const Stacks &obj)
{
  for (size_t i = 0; i < obj.GetSize(); ++i) {
	  const Stack &stack = obj.m_stacks[i];
	  out << stack.GetHypoSize() << " ";
  }

  return out;
}

void Stacks::Add(const Hypothesis *hypo, boost::object_pool<Hypothesis> &hypoPool)
{
	size_t numWordsCovered = hypo->GetBitmap().GetNumWordsCovered();
	//cerr << "numWordsCovered=" << numWordsCovered << endl;
	Stack &stack = m_stacks[numWordsCovered];
	stack.Add(hypo, hypoPool);

}

}


