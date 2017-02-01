/*
 * Stacks.cpp
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#include "Stacks.h"
#include "../Manager.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{

namespace NSNormal
{

Stacks::Stacks(const Manager &mgr) :
  m_mgr(mgr)
{
  // TODO Auto-generated constructor stub

}

Stacks::~Stacks()
{
  for (size_t i = 0; i < m_stacks.size(); ++i) {
    delete m_stacks[i];
  }
}

void Stacks::Init(const Manager &mgr, size_t numStacks)
{
  m_stacks.resize(numStacks);
  for (size_t i = 0; i < m_stacks.size(); ++i) {
    m_stacks[i] = new Stack(mgr);
  }
}

std::string Stacks::Debug(const System &system) const
{
  stringstream out;
  for (size_t i = 0; i < GetSize(); ++i) {
    const Stack *stack = m_stacks[i];
    if (stack) {
      out << stack->GetSize() << " ";
    } else {
      out << "N ";
    }
  }
  return out.str();
}

void Stacks::Add(Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
                 ArcLists &arcLists)
{
  size_t numWordsCovered = hypo->GetBitmap().GetNumWordsCovered();
  //cerr << "numWordsCovered=" << numWordsCovered << endl;
  Stack &stack = *m_stacks[numWordsCovered];
  stack.Add(m_mgr, hypo, hypoRecycle, arcLists);
}

}
}
