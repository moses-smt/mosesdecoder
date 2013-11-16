#include "search/nbest.hh"

#include "util/pool.hh"
#include "moses/Util.h"

#include <algorithm>
#include <functional>
#include <queue>

#include <assert.h>
#include <math.h>

namespace search {

NBestList::NBestList(std::vector<PartialEdge> &partials, util::Pool &entry_pool, std::size_t keep) {
  assert(!partials.empty());
  std::vector<PartialEdge>::iterator end;
  if (partials.size() > keep) {
    end = partials.begin() + keep;
    NTH_ELEMENT4(partials.begin(), end, partials.end(), std::greater<PartialEdge>());
  } else {
    end = partials.end();
  }
  for (std::vector<PartialEdge>::const_iterator i(partials.begin()); i != end; ++i) {
    queue_.push(QueueEntry(entry_pool.Allocate(QueueEntry::Size(i->GetArity())), *i));
  }
}

Score NBestList::TopAfterConstructor() const {
  assert(revealed_.empty());
  return queue_.top().GetScore();
}

const std::vector<Applied> &NBestList::Extract(util::Pool &pool, std::size_t n) {
  while (revealed_.size() < n && !queue_.empty()) {
    MoveTop(pool);
  }
  return revealed_;
}

Score NBestList::Visit(util::Pool &pool, std::size_t index) {
  if (index + 1 < revealed_.size())
    return revealed_[index + 1].GetScore() - revealed_[index].GetScore();
  if (queue_.empty()) 
    return -INFINITY;
  if (index + 1 == revealed_.size())
    return queue_.top().GetScore() - revealed_[index].GetScore();
  assert(index == revealed_.size());

  MoveTop(pool);

  if (queue_.empty()) return -INFINITY;
  return queue_.top().GetScore() - revealed_[index].GetScore();
}

Applied NBestList::Get(util::Pool &pool, std::size_t index) {
  assert(index <= revealed_.size());
  if (index == revealed_.size()) MoveTop(pool);
  return revealed_[index];
}

void NBestList::MoveTop(util::Pool &pool) {
  assert(!queue_.empty());
  QueueEntry entry(queue_.top());
  queue_.pop();
  RevealedRef *const children_begin = entry.Children();
  RevealedRef *const children_end = children_begin + entry.GetArity();
  Score basis = entry.GetScore();
  for (RevealedRef *child = children_begin; child != children_end; ++child) {
    Score change = child->in_->Visit(pool, child->index_);
    if (change != -INFINITY) {
      assert(change < 0.001);
      QueueEntry new_entry(pool.Allocate(QueueEntry::Size(entry.GetArity())), basis + change, entry.GetArity(), entry.GetNote());
      std::copy(children_begin, child, new_entry.Children());
      RevealedRef *update = new_entry.Children() + (child - children_begin);
      update->in_ = child->in_;
      update->index_ = child->index_ + 1;
      std::copy(child + 1, children_end, update + 1);
      queue_.push(new_entry);
    }
    // Gesmundo, A. and Henderson, J. Faster Cube Pruning, IWSLT 2010.
    if (child->index_) break;
  }

  // Convert QueueEntry to Applied.  This leaves some unused memory.  
  void *overwrite = entry.Children();
  for (unsigned int i = 0; i < entry.GetArity(); ++i) {
    RevealedRef from(*(static_cast<const RevealedRef*>(overwrite) + i));
    *(static_cast<Applied*>(overwrite) + i) = from.in_->Get(pool, from.index_);
  }
  revealed_.push_back(Applied(entry.Base()));
}

NBestComplete NBest::Complete(std::vector<PartialEdge> &partials) {
  assert(!partials.empty());
  NBestList *list = list_pool_.construct(partials, entry_pool_, config_.keep);
  return NBestComplete(
      list,
      partials.front().CompletedState(), // All partials have the same state
      list->TopAfterConstructor());
}

const std::vector<Applied> &NBest::Extract(History history) {
  return static_cast<NBestList*>(history)->Extract(entry_pool_, config_.size);
}

} // namespace search
