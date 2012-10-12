#include "Incremental/Fill.h"

#include "Incremental/Edge.h"
#include "Incremental/Owner.h"
#include "ChartCellLabel.h"
#include "ChartCellLabelSet.h"
#include "TargetPhraseCollection.h"
#include "TargetPhrase.h"
#include "Word.h"

#include "lm/model.hh"
#include "search/context.hh"
#include "search/vertex.hh"
#include "search/vertex_generator.hh"

namespace Moses {
namespace Incremental {

template <class Model> Fill<Model>::Fill(search::Context<Model> &context, const std::vector<lm::WordIndex> &vocab_mapping, Owner &owner) 
  : context_(context), vocab_mapping_(vocab_mapping), owner_(owner), edges_(context.PopLimit()) {}

template <class Model> void Fill<Model>::Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordsRange &) {
  const float word_multiplier = -0.434294482 * context_.GetWeights().WordPenalty();
  std::vector<const search::Vertex*> non_terminals;
  for (StackVec::const_iterator i = nts.begin(); i != nts.end(); ++i) {
    non_terminals.push_back((*i)->GetStack().incr);
  }

  for (TargetPhraseCollection::const_iterator i(targets.begin()); i != targets.end(); ++i) {
    const TargetPhrase &phrase = **i;
    std::vector<lm::WordIndex> words;
    size_t i = 0;
    bool bos = false;
    if (phrase.GetSize() && !phrase.GetWord(0).IsNonTerminal()) {
      lm::WordIndex index = Convert(phrase.GetWord(0));
      if (context_.LanguageModel().GetVocabulary().BeginSentence() == index) {
        bos = true;
      } else {
        words.push_back(index);
      }
      i = 1;
    }
    for (; i < phrase.GetSize(); ++i) {
      const Word &word = phrase.GetWord(i);
      if (word.IsNonTerminal()) {
        words.push_back(search::kNonTerminal);
      } else {
        words.push_back(Convert(word));
      }
    }
    Edge &edge = *owner_.EdgePool().construct(phrase);
    float score = phrase.GetTranslationScore() + word_multiplier * static_cast<float>(phrase.GetSize() - nts.size());
    score += edge.InitRule().Init(context_, words, bos);
    for (std::vector<const search::Vertex*>::const_iterator i(non_terminals.begin()); i != non_terminals.end(); ++i) {
      edge.Add(**i);
    }
    edges_.AddEdge(edge, score);
  }
}

template <class Model> void Fill<Model>::AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection*> &, const WordsRange &) {
  std::vector<lm::WordIndex> words;
  CHECK(phrase.GetSize() <= 1);
  if (phrase.GetSize())
    words.push_back(Convert(phrase.GetWord(0)));
  Edge &edge = *owner_.EdgePool().construct(phrase);
  edge.InitRule().Init(context_, words, false);
  // This includes the phrase OOV penalty and WordPenalty.  
  // Appears to be a bug that this does not include language model.  
  edges_.AddEdge(edge, phrase.GetFutureScore());
}

namespace {
// Route hypotheses to separate vertices for each left hand side label, populating ChartCellLabelSet out.  
class HypothesisCallback {
  public:
    HypothesisCallback(search::ContextBase &context, ChartCellLabelSet &out, boost::object_pool<search::Vertex> &vertex_pool)
      : context_(context), out_(out), vertex_pool_(vertex_pool) {}

    void NewHypothesis(const search::PartialEdge &partial, const search::Edge &from) {
      search::VertexGenerator *&entry = out_.FindOrInsert(static_cast<const Edge&>(from).GetMoses().GetTargetLHS()).incr_generator;
      if (!entry) {
        entry = generator_pool_.construct(context_, *vertex_pool_.construct());
      }
      entry->NewHypothesis(partial, from);
    }

    void FinishedSearch() {
      for (ChartCellLabelSet::iterator i(out_.mutable_begin()); i != out_.mutable_end(); ++i) {
        ChartCellLabel::Stack &stack = i->second.MutableStack();
        stack.incr_generator->FinishedSearch();
        stack.incr = &stack.incr_generator->Generating();
      }
    }

  private:
    search::ContextBase &context_;

    ChartCellLabelSet &out_;

    boost::object_pool<search::Vertex> &vertex_pool_;
    boost::object_pool<search::VertexGenerator> generator_pool_;
};
} // namespace

template <class Model> void Fill<Model>::Search(ChartCellLabelSet &out) {
  HypothesisCallback callback(context_, out, owner_.VertexPool());
  edges_.Search(context_, callback);
}

// TODO: factors (but chart doesn't seem to support factors anyway). 
template <class Model> lm::WordIndex Fill<Model>::Convert(const Word &word) const {
  std::size_t factor = word.GetFactor(0)->GetId();
  return (factor >= vocab_mapping_.size() ? 0 : vocab_mapping_[factor]);
}

template class Fill<lm::ngram::ProbingModel>;
template class Fill<lm::ngram::RestProbingModel>;
template class Fill<lm::ngram::TrieModel>;
template class Fill<lm::ngram::QuantTrieModel>;
template class Fill<lm::ngram::ArrayTrieModel>;
template class Fill<lm::ngram::QuantArrayTrieModel>;

} // namespace Incremental
} // namespace Moses
