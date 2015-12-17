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
//  cerr << "Deleting stacks size==" << m_stacksMap.size() << " maxSize==" << m_stacksMap.max_size() << " m_stacks.size()==" << m_stacks.size() << endl;
//	for (size_t i = 0; i < m_stacks.size(); ++i) {
//		delete m_stacks[i];
//	}
  
  for (auto& pair : m_stacksMap) {
//    cerr << "Deleting stack " << pair.first->GetNumWordsCovered() << " " << pair.first->GetSize() << endl;
    delete pair.second;
  }
}
//
//void Stacks::Init(size_t numStacks)
//{
//	m_stacks.resize(numStacks);
//	for (size_t i = 0; i < m_stacks.size(); ++i) {
//		m_stacks[i] = new Stack();
//	}
//}

bool Stacks::empty() {
  return m_queue.empty();
}

Stack* Stacks::getNextStack() {
  const Bitmap* key = m_queue.top();
  Stack* result = m_stacksMap[key];
  m_queue.pop();
  return result;
}

std::ostream& operator<<(std::ostream &out, const Stacks &obj)
{
//  for (size_t i = 0; i < obj.GetSize(); ++i) {
//	  const Stack *stack = obj.m_stacks[i];
//	  if (stack) {
//		  out << stack->GetSize() << " ";
//	  }
//	  else {
//		  out << "N ";
//	  }
//  }

  return out;
}

void Stacks::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
  const Bitmap &bitmap = hypo->GetBitmap();
//	size_t numWordsCovered = bitmap.GetNumWordsCovered();
	//cerr << "numWordsCovered=" << numWordsCovered << endl;
//	Stack &stack = *m_stacks[numWordsCovered];
//	stack.Add(hypo, hypoRecycle);

  auto search = m_stacksMap.find(&bitmap);
  Stack *stack;
  if (search == m_stacksMap.end()) {
//    cerr << "Creating new stack for coverage " << bitmap.GetNumWordsCovered() << " " << bitmap.GetSize() << endl;
    stack = new Stack();
    m_stacksMap.insert({&bitmap, stack});
    m_queue.push(&bitmap);
    m_lastStack = stack;
  } else {
    stack = search->second;
//    cerr << "bar" << endl;
  }
  
  
  stack->Add(hypo, hypoRecycle);
  
}

