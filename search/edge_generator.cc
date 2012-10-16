#include "search/edge_generator.hh"

#include "lm/left.hh"
#include "lm/partial.hh"
#include "search/context.hh"
#include "search/vertex.hh"
#include "search/vertex_generator.hh"

#include <numeric>

namespace search {

EdgeGenerator::EdgeGenerator(PartialEdge root, Note note) : top_score_(root.GetScore()), arity_(root.GetArity()), note_(note) {
  generate_.push(root);
}

namespace {

template <class Model> void FastScore(const Context<Model> &context, Arity victim, Arity before_idx, Arity incomplete, const PartialEdge previous, PartialEdge update) {
  lm::ngram::ChartState *between = update.Between();
  const lm::ngram::ChartState *previous_between = previous.Between();
  const search::PartialVertex &previous_vertex = previous.NT()[victim];

  lm::ngram::ChartState *before = &between[before_idx], *after = &between[before_idx + 1];
  // copy [0, after] 
  memcpy(between, previous_between, sizeof(lm::ngram::ChartState) * (before_idx + 2));

  float adjustment = 0.0;
  const lm::ngram::ChartState &previous_reveal = previous_vertex.State();
  const PartialVertex &update_nt = update.NT()[victim];
  const lm::ngram::ChartState &update_reveal = update_nt.State();
  if ((update_reveal.left.length > previous_reveal.left.length) || (update_reveal.left.full && !previous_reveal.left.full)) {
    adjustment += lm::ngram::RevealAfter(context.LanguageModel(), before->left, before->right, update_reveal.left, previous_reveal.left.length);
  }
  if ((update_reveal.right.length > previous_reveal.right.length) || (update_nt.RightFull() && !previous_vertex.RightFull())) {
    adjustment += lm::ngram::RevealBefore(context.LanguageModel(), update_reveal.right, previous_reveal.right.length, update_nt.RightFull(), after->left, after->right);
  }
  if (update_nt.Complete()) {
    if (update_reveal.left.full) {
      before->left.full = true;
    } else {
      assert(update_reveal.left.length == update_reveal.right.length);
      adjustment += lm::ngram::Subsume(context.LanguageModel(), before->left, before->right, after->left, after->right, update_reveal.left.length);
    }
    before->right = after->right;
    // Copy the others shifted one down, covering after.  
    memcpy(after, previous_between + before_idx + 2, sizeof(lm::ngram::ChartState) * (incomplete + 1 - before_idx - 2));
  } else {
    // Copy [after + 1, incomplete]
    memcpy(after + 1, previous_between + before_idx + 2, sizeof(lm::ngram::ChartState) * (incomplete + 1 - before_idx - 2));
  }
  update.SetScore(previous.GetScore() + adjustment * context.GetWeights().LM());
}

} // namespace

template <class Model> PartialEdge EdgeGenerator::Pop(Context<Model> &context, PartialEdgePool &partial_edge_pool) {
  assert(!generate_.empty());
  PartialEdge top = generate_.top();
  generate_.pop();
  PartialVertex *top_nt = top.NT();

  Arity victim = 0;
  Arity victim_completed;
  Arity completed = 0;
  // Select victim or return if complete.   
  {
    unsigned char lowest_length = 255;
    for (Arity i = 0; i != arity_; ++i) {
      if (top_nt[i].Complete()) {
        ++completed;
      } else if (top_nt[i].Length() < lowest_length) {
        lowest_length = top_nt[i].Length();
        victim = i;
        victim_completed = completed;
      }
    }
    if (lowest_length == 255) {
      // Now top.between[0] is the full edge state.  
      top_score_ = generate_.empty() ? -kScoreInf : generate_.top().GetScore();
      return top;
    }
  }

  float old_bound = top_nt[victim].Bound();
  PartialEdge continuation = partial_edge_pool.Allocate(arity_);
  PartialVertex *continuation_nt = continuation.NT();
  // The alternate's score will change because the nt changes.
  bool split = top_nt[victim].Split(continuation_nt[victim]);
  // top is now the alternate.  

  for (Arity i = 0; i < victim; ++i) continuation_nt[i] = top_nt[i];
  for (Arity i = victim + 1; i < arity_; ++i) continuation_nt[i] = top_nt[i];
  FastScore(context, victim, victim - victim_completed, arity_ - completed, top, continuation);
  // TODO: dedupe?  
  generate_.push(continuation);

  if (split) {
    // We have an alternate.  
    top.SetScore(top.GetScore() + top_nt[victim].Bound() - old_bound);
    // TODO: dedupe?  
    generate_.push(top);
  } else {
    // TODO should free top here. 
    // Better would be changing Split.  
  }

  top_score_ = generate_.top().GetScore();
  // Invalid indicates no new hypothesis generated.  
  return PartialEdge();
}

template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::RestProbingModel> &context, PartialEdgePool &partial_edge_pool);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::ProbingModel> &context, PartialEdgePool &partial_edge_pool);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::TrieModel> &context, PartialEdgePool &partial_edge_pool);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::QuantTrieModel> &context, PartialEdgePool &partial_edge_pool);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::ArrayTrieModel> &context, PartialEdgePool &partial_edge_pool);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::QuantArrayTrieModel> &context, PartialEdgePool &partial_edge_pool);

} // namespace search
