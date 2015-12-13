#include "search/edge_generator.hh"

#include "lm/left.hh"
#include "lm/model.hh"
#include "lm/partial.hh"
#include "search/context.hh"
#include "search/vertex.hh"

#include <numeric>

namespace search {

namespace {

template <class Model> void FastScore(const Context<Model> &context, Arity victim, Arity before_idx, Arity incomplete, const PartialVertex &previous_vertex, PartialEdge update) {
  lm::ngram::ChartState *between = update.Between();
  lm::ngram::ChartState *before = &between[before_idx], *after = &between[before_idx + 1];

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
    // Shift the others shifted one down, covering after.
    for (lm::ngram::ChartState *cover = after; cover < between + incomplete; ++cover) {
      *cover = *(cover + 1);
    }
  }
  update.SetScore(update.GetScore() + adjustment * context.LMWeight());
}

} // namespace

template <class Model> PartialEdge EdgeGenerator::Pop(Context<Model> &context) {
  assert(!generate_.empty());
  PartialEdge top = generate_.top();
  generate_.pop();
  PartialVertex *const top_nt = top.NT();
  const Arity arity = top.GetArity();

  Arity victim = 0;
  Arity victim_completed;
  Arity incomplete;
  unsigned char lowest_niceness = 255;
  // Select victim or return if complete.
  {
    Arity completed = 0;
    for (Arity i = 0; i != arity; ++i) {
      if (top_nt[i].Complete()) {
        ++completed;
      } else if (top_nt[i].Niceness() < lowest_niceness) {
        lowest_niceness = top_nt[i].Niceness();
        victim = i;
        victim_completed = completed;
      }
    }
    if (lowest_niceness == 255) {
      return top;
    }
    incomplete = arity - completed;
  }

  PartialVertex old_value(top_nt[victim]);
  PartialVertex alternate_changed;
  if (top_nt[victim].Split(alternate_changed)) {
    PartialEdge alternate(partial_edge_pool_, arity, incomplete + 1);
    alternate.SetScore(top.GetScore() + alternate_changed.Bound() - old_value.Bound());

    alternate.SetNote(top.GetNote());
    alternate.SetRange(top.GetRange());

    PartialVertex *alternate_nt = alternate.NT();
    for (Arity i = 0; i < victim; ++i) alternate_nt[i] = top_nt[i];
    alternate_nt[victim] = alternate_changed;
    for (Arity i = victim + 1; i < arity; ++i) alternate_nt[i] = top_nt[i];

    memcpy(alternate.Between(), top.Between(), sizeof(lm::ngram::ChartState) * (incomplete + 1));

    // TODO: dedupe?
    generate_.push(alternate);
  }

#ifndef NDEBUG
  Score before = top.GetScore();
#endif
  // top is now the continuation.
  FastScore(context, victim, victim - victim_completed, incomplete, old_value, top);
  // TODO: dedupe?
  generate_.push(top);
  assert(lowest_niceness != 254 || top.GetScore() == before);

  // Invalid indicates no new hypothesis generated.
  return PartialEdge();
}

template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::RestProbingModel> &context);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::ProbingModel> &context);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::TrieModel> &context);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::QuantTrieModel> &context);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::ArrayTrieModel> &context);
template PartialEdge EdgeGenerator::Pop(Context<lm::ngram::QuantArrayTrieModel> &context);

} // namespace search
