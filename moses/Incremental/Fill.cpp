#include "Fill.h"

#include "moses/ChartCellLabel.h"
#include "moses/ChartCellLabelSet.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/Word.h"

#include "lm/model.hh"
#include "search/context.hh"
#include "search/note.hh"
#include "search/rule.hh"
#include "search/vertex.hh"
#include "search/vertex_generator.hh"

#include <math.h>

namespace Moses {
namespace Incremental {

template <class Model> Fill<Model>::Fill(search::Context<Model> &context, const std::vector<lm::WordIndex> &vocab_mapping) 
  : context_(context), vocab_mapping_(vocab_mapping) {}

template <class Model> void Fill<Model>::Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordsRange &) {
  std::vector<search::PartialVertex> vertices;
  vertices.reserve(nts.size());
  float below_score = 0.0;
  for (StackVec::const_iterator i(nts.begin()); i != nts.end(); ++i) {
    vertices.push_back((*i)->GetStack().incr->RootPartial());
    if (vertices.back().Empty()) return;
    below_score += vertices.back().Bound();
  }

  std::vector<lm::WordIndex> words;
  for (TargetPhraseCollection::const_iterator p(targets.begin()); p != targets.end(); ++p) {
    words.clear();
    const TargetPhrase &phrase = **p;
    const AlignmentInfo::NonTermIndexMap &align = phrase.GetAlignNonTerm().GetNonTermIndexMap();
    search::PartialEdge edge(edges_.AllocateEdge(nts.size()));

    size_t i = 0;
    bool bos = false;
    search::PartialVertex *nt = edge.NT();
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
        *(nt++) = vertices[align[i]];
        words.push_back(search::kNonTerminal);
      } else {
        words.push_back(Convert(word));
      }
    }

    edge.SetScore(phrase.GetFutureScore() + below_score);
    search::ScoreRule(context_, words, bos, edge.Between());

    search::Note note;
    note.vp = &phrase;
    edge.SetNote(note);

    edges_.AddEdge(edge);
  }
}

template <class Model> void Fill<Model>::AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection*> &, const WordsRange &) {
  std::vector<lm::WordIndex> words;
  CHECK(phrase.GetSize() <= 1);
  if (phrase.GetSize())
    words.push_back(Convert(phrase.GetWord(0)));

  search::PartialEdge edge(edges_.AllocateEdge(0));
  // Appears to be a bug that FutureScore does not already include language model.  
  edge.SetScore(phrase.GetFutureScore() + search::ScoreRule(context_, words, false, edge.Between()));

  search::Note note;
  note.vp = &phrase;
  edge.SetNote(note);

  edges_.AddEdge(edge);
}

namespace {
// Route hypotheses to separate vertices for each left hand side label, populating ChartCellLabelSet out.  
class HypothesisCallback {
  public:
    HypothesisCallback(search::ContextBase &context, ChartCellLabelSet &out, boost::object_pool<search::Vertex> &vertex_pool)
      : context_(context), out_(out), vertex_pool_(vertex_pool) {}

    void NewHypothesis(search::PartialEdge partial) {
      search::VertexGenerator *&entry = out_.FindOrInsert(static_cast<const TargetPhrase *>(partial.GetNote().vp)->GetTargetLHS()).incr_generator;
      if (!entry) {
        entry = generator_pool_.construct(context_, *vertex_pool_.construct());
      }
      entry->NewHypothesis(partial);
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

template <class Model> void Fill<Model>::Search(ChartCellLabelSet &out, boost::object_pool<search::Vertex> &vertex_pool) {
  HypothesisCallback callback(context_, out, vertex_pool);
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
