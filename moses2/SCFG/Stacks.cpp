#include "Stacks.h"
#include "Stack.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
Stacks::~Stacks()
{
  for (size_t i = 0; i < m_cells.size(); ++i) {
    std::vector<Stack*> &inner = m_cells[i];
    for (size_t j = 0; j < inner.size(); ++j) {
      Stack *stack = inner[j];
      delete stack;
    }
  }
}

void Stacks::Init(SCFG::Manager &mgr, size_t size)
{
  m_cells.resize(size);
  for (size_t startPos = 0; startPos < size; ++startPos) {
    std::vector<Stack*> &inner = m_cells[startPos];
    inner.reserve(size - startPos);
    for (size_t endPos = startPos; endPos < size; ++endPos) {
      inner.push_back(new Stack(mgr));
    }
  }
}

void Stacks::OutputStacks() const
{
  size_t size = m_cells.size();

  for (size_t startPos = 0; startPos < size; ++startPos) {
    cerr.width(3);
    cerr << startPos << " ";
  }
  cerr << endl;
  for (size_t width = 1; width <= size; width++) {
    for( size_t space = 0; space < width-1; space++ ) {
      cerr << "  ";
    }
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      cerr.width(3);
      cerr << GetStack(startPos, width).GetSize() << " ";
    }
    cerr << endl;
  }

}

}
}
