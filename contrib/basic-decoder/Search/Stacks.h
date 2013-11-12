#pragma once

#include <vector>
#include "Stack.h"

class Hypothesis;

class Stacks
{
public:
  Stacks(size_t size);

  size_t GetSize() const
  { return m_stacks.size(); }

  const Stack &Get(size_t i) const
  { return m_stacks[i]; }
  Stack &Get(size_t i)
  { return m_stacks[i]; }

  const Stack &Back() const
  { return m_stacks.back(); }
  
  inline bool Add(Hypothesis *hypo, size_t wordsCovered)
  {
    bool added = m_stacks[wordsCovered].AddPrune(hypo);
    return added;
  }
  
protected:
  std::vector<Stack> m_stacks;

};


