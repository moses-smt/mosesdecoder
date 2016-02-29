#include "Stacks.h"
#include "Stack.h"

namespace Moses2
{

namespace SCFG
{

void Stacks::Init(Moses2::ManagerBase &mgr, size_t size)
{
    for (size_t startPos = 0; startPos < size; ++startPos) {
      std::vector<Stack*> &inner = m_cells[startPos];
      inner.reserve(size - startPos);
      for (size_t endPos = startPos; endPos < size; ++endPos) {
        inner.push_back(new Stack());
      }
    }
}

}
}
