#include "CubeQueue.h"

namespace Moses
{
namespace Syntax
{

CubeQueue::~CubeQueue()
{
  while (!m_queue.empty()) {
    Cube *cube = m_queue.top();
    m_queue.pop();
    delete cube;
  }
}

SHyperedge *CubeQueue::Pop()
{
  // pop the most promising cube
  Cube *cube = m_queue.top();
  m_queue.pop();

  // pop the most promising hyperedge from the cube
  SHyperedge *hyperedge = cube->Pop();

  // if the cube contains more items then push it back onto the queue
  if (!cube->IsEmpty()) {
    m_queue.push(cube);
  } else {
    delete cube;
  }

  return hyperedge;
}

}  // Syntax
}  // Moses
