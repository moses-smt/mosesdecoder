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
	void Init(Moses2::ManagerBase &mgr, size_t size);

protected:
	  std::vector<std::vector<Stack*> > m_cells;

};

}

}

