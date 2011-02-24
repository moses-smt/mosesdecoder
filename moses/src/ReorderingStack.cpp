/*
 * ReorderingStack.cpp
 ** Author: Ankit K. Srivastava
 ** Date: Jan 26, 2010
*/

#include "ReorderingStack.h"
#include <vector>

namespace Moses
{
int ReorderingStack::Compare(const ReorderingStack& o)  const
{
  const ReorderingStack& other = static_cast<const ReorderingStack&>(o);
  if (other.m_stack > m_stack) {
    return 1;
  } else if (other.m_stack < m_stack) {
    return -1;
  }
  return 0;
}

// Method to push (shift element into the stack and reduce if reqd)
int ReorderingStack::ShiftReduce(WordsRange input_span)
{
  int distance;  // value to return: the initial distance between this and previous span

  // stack is empty
  if(m_stack.empty()) {
    m_stack.push_back(input_span);
    return input_span.GetStartPos() + 1; // - (-1)
  }

  // stack is non-empty
  WordsRange prev_span = m_stack.back(); //access last element added

  //calculate the distance we are returning
  if(input_span.GetStartPos() > prev_span.GetStartPos()) {
    distance = input_span.GetStartPos() - prev_span.GetEndPos();
  } else {
    distance = input_span.GetEndPos() - prev_span.GetStartPos();
  }

  if(distance == 1) { //monotone
    m_stack.pop_back();
    WordsRange new_span(prev_span.GetStartPos(), input_span.GetEndPos());
    Reduce(new_span);
  } else if(distance == -1) { //swap
    m_stack.pop_back();
    WordsRange new_span(input_span.GetStartPos(), prev_span.GetEndPos());
    Reduce(new_span);
  } else {      // discontinuous
    m_stack.push_back(input_span);
  }

  return distance;
}

// Method to reduce, if possible the spans
void ReorderingStack::Reduce(WordsRange current)
{
  bool cont_loop = true;

  while (cont_loop && m_stack.size() > 0) {

    WordsRange previous = m_stack.back();

    if(current.GetStartPos() - previous.GetEndPos() == 1) { //mono&merge
      m_stack.pop_back();
      WordsRange t(previous.GetStartPos(), current.GetEndPos());
      current = t;
    } else if(previous.GetStartPos() - current.GetEndPos() == 1) { //swap&merge
      m_stack.pop_back();
      WordsRange t(current.GetStartPos(), previous.GetEndPos());
      current = t;
    } else { // discontinuous, no more merging
      cont_loop=false;
    }
  } // finished reducing, exit

  // add to stack
  m_stack.push_back(current);
}

}

