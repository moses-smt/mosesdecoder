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
  void Init(SCFG::Manager &mgr, size_t size);

  Stack &GetStack(size_t startPos, size_t size)
  { return *m_cells[startPos][size - 1]; }

protected:
  std::vector<std::vector<Stack*> > m_cells;

};

}

}

