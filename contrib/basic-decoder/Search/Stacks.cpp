#include "Stacks.h"

Stacks::Stacks(size_t size)
:m_stacks(size)
{
  for (size_t i = 0; i <= m_stacks.size(); ++i) {
    m_stacks[i].SetContainer(*this);
  }
}
/*
bool Stacks::Add(Hypothesis *hypo, size_t wordsCovered)
{
  bool added = m_stacks[wordsCovered].AddPrune(hypo);
  return added;
}
*/
