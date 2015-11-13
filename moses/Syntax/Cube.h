#pragma once

#include <queue>
#include <vector>
#include <utility>

#include <boost/unordered_set.hpp>

#include "SHyperedge.h"
#include "SHyperedgeBundle.h"

namespace Moses
{
namespace Syntax
{

// A cube -- in the cube pruning sense (see Chiang (2007)) -- that lazily
// produces SHyperedge objects from a SHyperedgeBundle in approximately
// best-first order.
class Cube
{
public:
  Cube(const SHyperedgeBundle &);
  ~Cube();

  SHyperedge *Pop();

  SHyperedge *Top() const {
    return m_queue.top().first;
  }

  bool IsEmpty() const {
    return m_queue.empty();
  }

private:
  typedef boost::unordered_set<std::vector<int> > CoordinateSet;

  typedef std::pair<SHyperedge *, const std::vector<int> *> QueueItem;

  class QueueItemOrderer
  {
  public:
    bool operator()(const QueueItem &p, const QueueItem &q) const {
      return p.first->label.score < q.first->label.score;
    }
  };

  typedef std::priority_queue<QueueItem, std::vector<QueueItem>,
          QueueItemOrderer> Queue;

  SHyperedge *CreateHyperedge(const std::vector<int> &);
  void CreateNeighbour(const std::vector<int> &);
  void CreateNeighbours(const std::vector<int> &);

  const SHyperedgeBundle &m_bundle;
  CoordinateSet m_visited;
  Queue m_queue;
};

}  // Syntax
}  // Moses
