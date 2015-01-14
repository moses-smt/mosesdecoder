#pragma once

#include <queue>
#include <vector>

#include "Cube.h"
#include "SHyperedge.h"
#include "SHyperedgeBundle.h"

namespace Moses
{
namespace Syntax
{

class CubeQueue
{
public:
  template<typename InputIterator>
  CubeQueue(InputIterator, InputIterator);

  ~CubeQueue();

  SHyperedge *Pop();

  bool IsEmpty() const {
    return m_queue.empty();
  }

private:
  class CubeOrderer
  {
  public:
    bool operator()(const Cube *p, const Cube *q) const {
      return p->Top()->score < q->Top()->score;
    }
  };

  typedef std::priority_queue<Cube*, std::vector<Cube*>, CubeOrderer> Queue;

  Queue m_queue;
};

template<typename InputIterator>
CubeQueue::CubeQueue(InputIterator first, InputIterator last)
{
  while (first != last) {
    m_queue.push(new Cube(*first++));
  }
}

}  // Syntax
}  // Moses
