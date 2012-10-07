#include "Incremental/Fill.h"

#include "Incremental/Edge.h"
#include "ChartCellLabel.h"
#include "TargetPhraseCollection.h"
#include "TargetPhrase.h"
#include "Word.h"

#include "lm/model.hh"
#include "search/context.hh"
#include "search/vertex.hh"

namespace Moses {
namespace Incremental {

template <class Model> Fill<Model>::Fill(search::Context<Model> &context, const std::vector<lm::WordIndex> &vocab_mapping, boost::object_pool<Edge> &edge_pool) 
  : context_(context), vocab_mapping_(vocab_mapping), edges_(context.PopLimit()), edge_pool_(edge_pool) {}

template <class Model> void Fill<Model>::Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordsRange &) {
  float word_multiplier = -0.434294482 * context_.GetWeights().WordPenalty();
  std::vector<const search::Vertex*> non_terminals;
  for (StackVec::const_iterator i = nts.begin(); i != nts.end(); ++i) {
    non_terminals.push_back(static_cast<const search::Vertex*>((*i)->GetStack()));
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
        words.push_back(search::Rule::kNonTerminal);
      } else {
        words.push_back(Convert(word));
      }
    }
    float additive = phrase.GetTranslationScore() + word_multiplier * static_cast<float>(phrase.GetSize() - nts.size());
    Edge &edge = *edge_pool_.construct(phrase);
    edge.InitRule().Init(context_, additive, words, bos);
    for (std::vector<const search::Vertex*>::const_iterator i(non_terminals.begin()); i != non_terminals.end(); ++i) {
      edge.Add(**i);
    }
    edges_.AddEdge(edge);
  }
}

// TODO: factors (but chart doesn't seem to support factors anyway). 
template <class Model> lm::WordIndex Fill<Model>::Convert(const Word &word) const {
  std::size_t factor = word.GetFactor(0)->GetId();
  return (factor >= vocab_mapping_.size() ? 0 : vocab_mapping_[factor]);
}

template class Fill<lm::ngram::RestProbingModel>;
template class Fill<lm::ngram::ProbingModel>;

} // namespace Incremental
} // namespace Moses
