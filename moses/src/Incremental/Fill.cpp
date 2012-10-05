#include "Incremental/Edge.h"
#include "Incremental/Fill.h"

namespace Moses {
namespace Incremental {

void Fill::Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordRange &ignored) {
  float word_multiplier = -0.434294482 * context_.GetWeights().WordPenalty();
  std::vector<const Vertex*> non_terminals;
  for (StackVec::const_iterator i = nts.begin(); i != nts.end(); ++i) {
    non_terminals.push_back(static_cast<const Vertex*>((*i)->GetStack()));
  }

  for (TargetPhraseCollection::const_iterator i(targets.begin()); i != targets.end(); ++i) {
    const TargetPhrase &phrase = **i;
    std::vector<lm::WordIndex> words;
    size_t i = 0;
    bool bos = false;
    if (phrase.GetSize() && !phrase.GetWord(0).IsNonTerminal()) {
      lm::WordIndex index = convert(phrase.GetWord(0));
      if (context_.LanguageModel().GetVocabulary().BeginSentence() == index) {
        bos = true;
      } else {
        words.push_back(word);
      }
      i = 1;
    }
    for (; i < phrase.GetSize(); ++i) {
      const Word &word = phrase.GetWord(i);
      if (word.IsNonTerminal()) {
        words.push_back(search::Rule::kNonTerminal);
      } else {
        // TODO: convert.  
        words.push_back(convert(word));
      }
    }
    float additive = phrase.GetTranslationScore() + word_multiplier * static_cast<float>(phrase.GetSize() - nts.size());
    Edge &edge = *edge_pool_.construct(phrase);
    edge.InitRule().Init(context_, additive, words, bos);
    for (std::vector<const Vertex*>::const_iterator i(non_terminals.begin()); i != non_terminals.end(); ++i) {
      edge.Add(**i);
    }
    edges_.AddEdge(edge);
  }
}

} // namespace Incremental
} // namespace Moses
