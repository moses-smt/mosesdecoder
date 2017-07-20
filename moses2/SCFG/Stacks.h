#pragma once

#include <stddef.h>
#include <vector>
#include "Stack.h"

namespace Moses2
{
class ManagerBase;

namespace SCFG
{
class Stacks
{
public:
  virtual ~Stacks();

  void Init(SCFG::Manager &mgr, size_t size);

  const Stack &GetStack(size_t startPos, size_t size) const {
    return *m_cells[startPos][size - 1];
  }

  Stack &GetStack(size_t startPos, size_t size) {
    return *m_cells[startPos][size - 1];
  }

  void OutputStacks() const;

  const Stack &GetLastStack() const {
    return GetStack(0, m_cells.size());
  }

protected:
  std::vector<std::vector<Stack*> > m_cells;

};

}

}

