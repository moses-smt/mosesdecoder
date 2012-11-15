#include "Manager.h"

#include "moses/ChartCell.h"
#include "moses/ChartParserCallback.h"
#include "moses/TranslationSystem.h"
#include "moses/StaticData.h"

#include "lm/model.hh"
#include "search/applied.hh"
#include "search/config.hh"
#include "search/context.hh"
#include "search/edge_generator.hh"
#include "search/rule.hh"
#include "search/vertex_generator.hh"

#include <boost/lexical_cast.hpp>

namespace Moses {
namespace Incremental {
namespace {

// This is called by EdgeGenerator.  Route hypotheses to separate vertices for
// each left hand side label, populating ChartCellLabelSet out.  
template <class Best> class HypothesisCallback {
  private:
    typedef search::VertexGenerator<Best> Gen;
  public:
    HypothesisCallback(search::ContextBase &context, Best &best, ChartCellLabelSet &out, boost::object_pool<search::Vertex> &vertex_pool)
      : context_(context), best_(best), out_(out), vertex_pool_(vertex_pool) {}

    void NewHypothesis(search::PartialEdge partial) {
      // Get the LHS, look it up in the output ChartCellLabel, and upcast it.  
      // It's not part of the union because it would have been ugly to expose template types in ChartCellLabel.
      ChartCellLabel::Stack &stack = out_.FindOrInsert(static_cast<const TargetPhrase *>(partial.GetNote().vp)->GetTargetLHS());
      Gen *entry = static_cast<Gen*>(stack.incr_generator);
      if (!entry) {
        entry = generator_pool_.construct(context_, *vertex_pool_.construct(), best_);
        stack.incr_generator = entry;
      }
      entry->NewHypothesis(partial);
    }

    void FinishedSearch() {
      for (ChartCellLabelSet::iterator i(out_.mutable_begin()); i != out_.mutable_end(); ++i) {
        ChartCellLabel::Stack &stack = i->second.MutableStack();
        Gen *gen = static_cast<Gen*>(stack.incr_generator);
        gen->FinishedSearch();
        stack.incr = &gen->Generating();
      }
    }

  private:
    search::ContextBase &context_;

    Best &best_;

    ChartCellLabelSet &out_;

    boost::object_pool<search::Vertex> &vertex_pool_;
    boost::object_pool<Gen> generator_pool_;
};

// This is called by the moses parser to collect hypotheses.  It converts to my
// edges (search::PartialEdge).  
template <class Model> class Fill : public ChartParserCallback {
  public:
    Fill(search::Context<Model> &context, const std::vector<lm::WordIndex> &vocab_mapping, search::Score oov_weight)
      : context_(context), vocab_mapping_(vocab_mapping), oov_weight_(oov_weight) {}

    void Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordsRange &ignored);

    void AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection*> &waste_memory, const WordsRange &range);

    bool Empty() const { return edges_.Empty(); }

    template <class Best> void Search(Best &best, ChartCellLabelSet &out, boost::object_pool<search::Vertex> &vertex_pool) {
      HypothesisCallback<Best> callback(context_, best, out, vertex_pool);
      edges_.Search(context_, callback);
    }

  private:
    lm::WordIndex Convert(const Word &word) const;

    search::Context<Model> &context_;

    const std::vector<lm::WordIndex> &vocab_mapping_;

    search::EdgeGenerator edges_;

    const search::Score oov_weight_;
};

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

    search::PartialVertex *nt = edge.NT();
    for (size_t i = 0; i < phrase.GetSize(); ++i) {
      const Word &word = phrase.GetWord(i);
      if (word.IsNonTerminal()) {
        *(nt++) = vertices[align[i]];
        words.push_back(search::kNonTerminal);
      } else {
        words.push_back(Convert(word));
      }
    }

    edge.SetScore(phrase.GetFutureScore() + below_score);
    // prob and oov were already accounted for.  
    search::ScoreRule(context_.LanguageModel(), words, edge.Between());

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
  search::ScoreRuleRet scored(search::ScoreRule(context_.LanguageModel(), words, edge.Between()));
  edge.SetScore(phrase.GetFutureScore() + scored.prob * context_.LMWeight() + static_cast<search::Score>(scored.oov) * oov_weight_);

  search::Note note;
  note.vp = &phrase;
  edge.SetNote(note);

  edges_.AddEdge(edge);
}

// TODO: factors (but chart doesn't seem to support factors anyway). 
template <class Model> lm::WordIndex Fill<Model>::Convert(const Word &word) const {
  std::size_t factor = word.GetFactor(0)->GetId();
  return (factor >= vocab_mapping_.size() ? 0 : vocab_mapping_[factor]);
}

struct ChartCellBaseFactory {
  ChartCellBase *operator()(size_t startPos, size_t endPos) const {
    return new ChartCellBase(startPos, endPos);
  }
};

} // namespace

Manager::Manager(const InputType &source, const TranslationSystem &system) :
  source_(source),
  system_(system),
  cells_(source, ChartCellBaseFactory()),
  parser_(source, system, cells_) {}

Manager::~Manager() {
  system_.CleanUpAfterSentenceProcessing(source_);
}

namespace {

void ConstructString(const search::Applied final, std::ostringstream &stream) {
  assert(final.Valid());
  const TargetPhrase &phrase = *static_cast<const TargetPhrase*>(final.GetNote().vp);
  size_t child = 0;
  for (std::size_t i = 0; i < phrase.GetSize(); ++i) {
    const Word &word = phrase.GetWord(i);
    if (word.IsNonTerminal()) {
      assert(child < final.GetArity());
      ConstructString(final.Children()[child++], stream);
    } else {
      stream << word[0]->GetString() << ' ';
    }
  }
}

void BestString(const ChartCellLabelSet &labels, std::string &out) {
  search::Applied best;
  for (ChartCellLabelSet::const_iterator i = labels.begin(); i != labels.end(); ++i) {
    const search::Applied child(i->second.GetStack().incr->BestChild());
    if (child.Valid() && (!best.Valid() || (child.GetScore() > best.GetScore()))) {
      best = child;
    }
  }
  if (!best.Valid()) {
    out.clear();
    return;
  }
  std::ostringstream stream;
  ConstructString(best, stream);
  out = stream.str();
  CHECK(out.size() > 9);
  // <s>
  out.erase(0, 4);
  // </s>
  out.erase(out.size() - 5);
  // Hack: include model score
  out += " ||| ";
  out += boost::lexical_cast<std::string>(best.GetScore());
}

} // namespace


template <class Model> void Manager::LMCallback(const Model &model, const std::vector<lm::WordIndex> &words) {
  const LanguageModel &abstract = **system_.GetLanguageModels().begin();
  const float oov_weight = abstract.OOVFeatureEnabled() ? abstract.GetOOVWeight() : 0.0;
  const StaticData &data = StaticData::Instance();
  search::Config config(abstract.GetWeight(), data.GetCubePruningPopLimit(), search::NBestConfig(data.GetNBestSize()));
  search::Context<Model> context(config, model);

  search::SingleBest best;

  size_t size = source_.GetSize();
  boost::object_pool<search::Vertex> vertex_pool(std::max<size_t>(size * size / 2, 32));
  

  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);
      Fill<Model> filler(context, words, oov_weight);
      parser_.Create(range, filler);
      filler.Search(best, cells_.MutableBase(range).MutableTargetLabelSet(), vertex_pool);
    }
  }
  BestString(cells_.GetBase(WordsRange(0, source_.GetSize() - 1)).GetTargetLabelSet(), output_);
}

template void Manager::LMCallback<lm::ngram::ProbingModel>(const lm::ngram::ProbingModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::RestProbingModel>(const lm::ngram::RestProbingModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::TrieModel>(const lm::ngram::TrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::QuantTrieModel>(const lm::ngram::QuantTrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::ArrayTrieModel>(const lm::ngram::ArrayTrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::QuantArrayTrieModel>(const lm::ngram::QuantArrayTrieModel &model, const std::vector<lm::WordIndex> &words);

void Manager::ProcessSentence() {
  const LMList &lms = system_.GetLanguageModels();
  UTIL_THROW_IF(lms.size() != 1, util::Exception, "Incremental search only supports one language model.");
  (*lms.begin())->IncrementalCallback(*this);
}

} // namespace Incremental
} // namespace Moses
