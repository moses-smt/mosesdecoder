#include "search/edge_queue.hh"

#include "lm/left.hh"
#include "search/context.hh"

#include <stdint.h>

namespace search {

EdgeQueue::EdgeQueue(unsigned int pop_limit_hint) : partial_edge_pool_(sizeof(PartialEdge), pop_limit_hint * 2) {
  take_ = static_cast<PartialEdge*>(partial_edge_pool_.malloc());
}

/*void EdgeQueue::AddEdge(PartialEdge &root, unsigned char arity, Note note) {
  // Ignore empty edges.  
  for (unsigned char i = 0; i < edge.Arity(); ++i) {
    PartialVertex root(edge.GetVertex(i).RootPartial());
    if (root.Empty()) return;
    total_score += root.Bound();
  }
  PartialEdge &allocated = *static_cast<PartialEdge*>(partial_edge_pool_.malloc());
  allocated.score = total_score;
}*/

} // namespace search
