/*
 * Stacks.cpp
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#include "Stacks.h"

using namespace std;

Stacks::Stacks() {
	// TODO Auto-generated constructor stub

}

Stacks::~Stacks() {
	for (size_t i = 0; i < m_stacks.size(); ++i) {
		delete m_stacks[i];
	}
}

void Stacks::Init(size_t numStacks)
{
	m_stacks.resize(numStacks);
	for (size_t i = 0; i < m_stacks.size(); ++i) {
		m_stacks[i] = new Stack();
	}
}

std::ostream& operator<<(std::ostream &out, const Stacks &obj)
{
  for (size_t i = 0; i < obj.GetSize(); ++i) {
	  const Stack *stack = obj.m_stacks[i];
	  if (stack) {
		  out << stack->GetSize() << " ";
	  }
	  else {
		  out << "N ";
	  }
  }

  return out;
}
