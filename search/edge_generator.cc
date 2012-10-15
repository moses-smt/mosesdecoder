#include "search/edge_generator.hh"

#include "lm/left.hh"
#include "lm/partial.hh"
#include "search/context.hh"
#include "search/vertex.hh"
#include "search/vertex_generator.hh"

#include <numeric>

namespace search {

EdgeGenerator::EdgeGenerator(PartialEdge &root, unsigned char arity, Note note) : arity_(arity), note_(note) {
/*  for (unsigned char i = 0; i < edge.Arity(); ++i) {
    root.nt[i] = edge.GetVertex(i).RootPartial();
  }
  for (unsigned char i = edge.Arity(); i < 2; ++i) {
    root.nt[i] = kBlankPartialVertex;
  }*/
  generate_.push(&root);
  top_score_ = root.score;
}

namespace {

template <class Model> float FastScore(const Context<Model> &context, unsigned char victim, unsigned char arity, const PartialEdge &previous, PartialEdge &update) {
  memcpy(update.between, previous.between, sizeof(lm::ngram::ChartState) * (arity + 1));

  float ret = 0.0;
  lm::ngram::ChartState *before, *after;
  if (victim == 0) {
    before = &update.between[0];
    after = &update.between[(arity == 2 && previous.nt[1].Complete()) ? 2 : 1];
  } else {
    assert(victim == 1);
    assert(arity == 2);
    before = &update.between[previous.nt[0].Complete() ? 0 : 1];
    after = &update.between[2];
  }
  const lm::ngram::ChartState &previous_reveal = previous.nt[victim].State();
  const PartialVertex &update_nt = update.nt[victim];
  const lm::ngram::ChartState &update_reveal = update_nt.State();
  float just_after = 0.0;
  if ((update_reveal.left.length > previous_reveal.left.length) || (update_reveal.left.full && !previous_reveal.left.full)) {
    just_after += lm::ngram::RevealAfter(context.LanguageModel(), before->left, before->right, update_reveal.left, previous_reveal.left.length);
  }
  if ((update_reveal.right.length > previous_reveal.right.length) || (update_nt.RightFull() && !previous.nt[victim].RightFull())) {
    ret += lm::ngram::RevealBefore(context.LanguageModel(), update_reveal.right, previous_reveal.right.length, update_nt.RightFull(), after->left, after->right);
  }
  if (update_nt.Complete()) {
    if (update_reveal.left.full) {
      before->left.full = true;
    } else {
      assert(update_reveal.left.length == update_reveal.right.length);
      ret += lm::ngram::Subsume(context.LanguageModel(), before->left, before->right, after->left, after->right, update_reveal.left.length);
    }
    if (victim == 0) {
      update.between[0].right = after->right;
    } else {
      update.between[2].left = before->left;
    }
  }
  return previous.score + (ret + just_after) * context.GetWeights().LM();
}

} // namespace

template <class Model> PartialEdge *EdgeGenerator::Pop(Context<Model> &context, boost::pool<> &partial_edge_pool) {
  assert(!generate_.empty());
  PartialEdge &top = *generate_.top();
  generate_.pop();
  unsigned int victim = 0;
  unsigned char lowest_length = 255;
  for (unsigned char i = 0; i != arity_; ++i) {
    if (!top.nt[i].Complete() && top.nt[i].Length() < lowest_length) {
      lowest_length = top.nt[i].Length();
      victim = i;
    }
  }
  if (lowest_length == 255) {
    // All states report complete.  
    top.between[0].right = top.between[arity_].right;
    // Now top.between[0] is the full edge state.  
    top_score_ = generate_.empty() ? -kScoreInf : generate_.top()->score;
    return &top;
  }

  unsigned int stay = !victim;
  PartialEdge &continuation = *static_cast<PartialEdge*>(partial_edge_pool.malloc());
  float old_bound = top.nt[victim].Bound();
  // The alternate's score will change because alternate.nt[victim] changes.  
  bool split = top.nt[victim].Split(continuation.nt[victim]);
  // top is now the alternate.  

  continuation.nt[stay] = top.nt[stay];
  continuation.score = FastScore(context, victim, arity_, top, continuation);
  // TODO: dedupe?  
  generate_.push(&continuation);

  if (split) {
    // We have an alternate.  
    top.score += top.nt[victim].Bound() - old_bound;
    // TODO: dedupe?  
    generate_.push(&top);
  } else {
    partial_edge_pool.free(&top);
  }

  top_score_ = generate_.top()->score;
  return NULL;
}

template PartialEdge *EdgeGenerator::Pop(Context<lm::ngram::RestProbingModel> &context, boost::pool<> &partial_edge_pool);
template PartialEdge *EdgeGenerator::Pop(Context<lm::ngram::ProbingModel> &context, boost::pool<> &partial_edge_pool);
template PartialEdge *EdgeGenerator::Pop(Context<lm::ngram::TrieModel> &context, boost::pool<> &partial_edge_pool);
template PartialEdge *EdgeGenerator::Pop(Context<lm::ngram::QuantTrieModel> &context, boost::pool<> &partial_edge_pool);
template PartialEdge *EdgeGenerator::Pop(Context<lm::ngram::ArrayTrieModel> &context, boost::pool<> &partial_edge_pool);
template PartialEdge *EdgeGenerator::Pop(Context<lm::ngram::QuantArrayTrieModel> &context, boost::pool<> &partial_edge_pool);

} // namespace search
