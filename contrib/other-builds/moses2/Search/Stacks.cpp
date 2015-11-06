/*
 * Stacks.cpp
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#include "Stacks.h"

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
