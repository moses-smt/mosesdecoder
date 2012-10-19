#include "search/vertex.hh"

#include "search/context.hh"

#include <algorithm>
#include <functional>

#include <assert.h>

namespace search {

namespace {

struct GreaterByBound : public std::binary_function<const VertexNode *, const VertexNode *, bool> {
  bool operator()(const VertexNode *first, const VertexNode *second) const {
    return first->Bound() > second->Bound();
  }
};

} // namespace

void VertexNode::SortAndSet(ContextBase &context, VertexNode **parent_ptr) {
  if (Complete()) {
    assert(end_.Valid());
    assert(extend_.empty());
    bound_ = end_.GetScore();
    return;
  }
  if (extend_.size() == 1 && parent_ptr) {
    *parent_ptr = extend_[0];
    extend_[0]->SortAndSet(context, parent_ptr);
    context.DeleteVertexNode(this);
    return;
  }
  for (std::vector<VertexNode*>::iterator i = extend_.begin(); i != extend_.end(); ++i) {
    (*i)->SortAndSet(context, &*i);
  }
  std::sort(extend_.begin(), extend_.end(), GreaterByBound());
  bound_ = extend_.front()->Bound();
}

} // namespace search
