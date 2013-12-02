#include <stdexcept>

#include "moses/Incremental.h"

#include "moses/ChartCell.h"
#include "moses/ChartParserCallback.h"
#include "moses/FeatureVector.h"
#include "moses/StaticData.h"
#include "moses/Util.h"
#include "moses/LM/Base.h"

#include "lm/model.hh"
#include "search/applied.hh"
#include "search/config.hh"
#include "search/context.hh"
#include "search/edge_generator.hh"
#include "search/rule.hh"
#include "search/vertex_generator.hh"

#include <boost/lexical_cast.hpp>

namespace Moses
{
namespace Incremental
{
namespace
{

// This is called by EdgeGenerator.  Route hypotheses to separate vertices for
// each left hand side label, populating ChartCellLabelSet out.
template <class Best> class HypothesisCallback
{
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
template <class Model> class Fill : public ChartParserCallback
{
public:
  Fill(search::Context<Model> &context, const std::vector<lm::WordIndex> &vocab_mapping, search::Score oov_weight)
    : context_(context), vocab_mapping_(vocab_mapping), oov_weight_(oov_weight) {}

  void Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordsRange &ignored);

  void AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection*> &waste_memory, const WordsRange &range);

  bool Empty() const {
    return edges_.Empty();
  }

  template <class Best> void Search(Best &best, ChartCellLabelSet &out, boost::object_pool<search::Vertex> &vertex_pool) {
    HypothesisCallback<Best> callback(context_, best, out, vertex_pool);
    edges_.Search(context_, callback);
  }

  // Root: everything into one vertex.
  template <class Best> search::History RootSearch(Best &best) {
    search::Vertex vertex;
    search::RootVertexGenerator<Best> gen(vertex, best);
    edges_.Search(context_, gen);
    return vertex.BestChild();
  }

  void Evaluate(const InputType &input, const InputPath &inputPath) {
    // TODO for input lattice
  }
private:
  lm::WordIndex Convert(const Word &word) const;

  search::Context<Model> &context_;

  const std::vector<lm::WordIndex> &vocab_mapping_;

  search::EdgeGenerator edges_;

  const search::Score oov_weight_;
};

template <class Model> void Fill<Model>::Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordsRange &)
{
  std::vector<search::PartialVertex> vertices;
  vertices.reserve(nts.size());
  float below_score = 0.0;
  for (StackVec::const_iterator i(nts.begin()); i != nts.end(); ++i) {
    vertices.push_back((*i)->GetStack().incr->RootAlternate());
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

template <class Model> void Fill<Model>::AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection*> &, const WordsRange &)
{
  std::vector<lm::WordIndex> words;
  UTIL_THROW_IF2(phrase.GetSize() > 1,
		  "OOV target phrase should be 0 or 1 word in length");
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
template <class Model> lm::WordIndex Fill<Model>::Convert(const Word &word) const
{
  std::size_t factor = word.GetFactor(0)->GetId();
  return (factor >= vocab_mapping_.size() ? 0 : vocab_mapping_[factor]);
}

struct ChartCellBaseFactory {
  ChartCellBase *operator()(size_t startPos, size_t endPos) const {
    return new ChartCellBase(startPos, endPos);
  }
};

} // namespace

Manager::Manager(const InputType &source) :
  source_(source),
  cells_(source, ChartCellBaseFactory()),
  parser_(source, cells_),
  n_best_(search::NBestConfig(StaticData::Instance().GetNBestSize())) {}

Manager::~Manager()
{
}

template <class Model, class Best> search::History Manager::PopulateBest(const Model &model, const std::vector<lm::WordIndex> &words, Best &out)
{
  const LanguageModel &abstract = LanguageModel::GetFirstLM();
  const float oov_weight = abstract.OOVFeatureEnabled() ? abstract.GetOOVWeight() : 0.0;
  const StaticData &data = StaticData::Instance();
  search::Config config(abstract.GetWeight() * M_LN10, data.GetCubePruningPopLimit(), search::NBestConfig(data.GetNBestSize()));
  search::Context<Model> context(config, model);

  size_t size = source_.GetSize();
  boost::object_pool<search::Vertex> vertex_pool(std::max<size_t>(size * size / 2, 32));

  for (size_t width = 1; width < size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      WordsRange range(startPos, startPos + width - 1);
      Fill<Model> filler(context, words, oov_weight);
      parser_.Create(range, filler);
      filler.Search(out, cells_.MutableBase(range).MutableTargetLabelSet(), vertex_pool);
    }
  }

  WordsRange range(0, size - 1);
  Fill<Model> filler(context, words, oov_weight);
  parser_.Create(range, filler);
  return filler.RootSearch(out);
}

template <class Model> void Manager::LMCallback(const Model &model, const std::vector<lm::WordIndex> &words)
{
  std::size_t nbest = StaticData::Instance().GetNBestSize();
  if (nbest <= 1) {
    search::History ret = PopulateBest(model, words, single_best_);
    if (ret) {
      backing_for_single_.resize(1);
      backing_for_single_[0] = search::Applied(ret);
    } else {
      backing_for_single_.clear();
    }
    completed_nbest_ = &backing_for_single_;
  } else {
    search::History ret = PopulateBest(model, words, n_best_);
    if (ret) {
      completed_nbest_ = &n_best_.Extract(ret);
    } else {
      backing_for_single_.clear();
      completed_nbest_ = &backing_for_single_;
    }
  }
}

template void Manager::LMCallback<lm::ngram::ProbingModel>(const lm::ngram::ProbingModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::RestProbingModel>(const lm::ngram::RestProbingModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::TrieModel>(const lm::ngram::TrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::QuantTrieModel>(const lm::ngram::QuantTrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::ArrayTrieModel>(const lm::ngram::ArrayTrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::QuantArrayTrieModel>(const lm::ngram::QuantArrayTrieModel &model, const std::vector<lm::WordIndex> &words);

const std::vector<search::Applied> &Manager::ProcessSentence()
{
  LanguageModel::GetFirstLM().IncrementalCallback(*this);
  return *completed_nbest_;
}

namespace
{

struct NoOp {
  void operator()(const TargetPhrase &) const {}
};
struct AccumScore {
  AccumScore(ScoreComponentCollection &out) : out_(&out) {}
  void operator()(const TargetPhrase &phrase) {
    out_->PlusEquals(phrase.GetScoreBreakdown());
  }
  ScoreComponentCollection *out_;
};
template <class Action> void AppendToPhrase(const search::Applied final, Phrase &out, Action action)
{
  assert(final.Valid());
  const TargetPhrase &phrase = *static_cast<const TargetPhrase*>(final.GetNote().vp);
  action(phrase);
  const search::Applied *child = final.Children();
  for (std::size_t i = 0; i < phrase.GetSize(); ++i) {
    const Word &word = phrase.GetWord(i);
    if (word.IsNonTerminal()) {
      AppendToPhrase(*child++, out, action);
    } else {
      out.AddWord(word);
    }
  }
}

} // namespace

void ToPhrase(const search::Applied final, Phrase &out)
{
  out.Clear();
  AppendToPhrase(final, out, NoOp());
}

void PhraseAndFeatures(const search::Applied final, Phrase &phrase, ScoreComponentCollection &features)
{
  phrase.Clear();
  features.ZeroAll();
  AppendToPhrase(final, phrase, AccumScore(features));

  // If we made it this far, there is only one language model.
  float full, ignored_ngram;
  std::size_t ignored_oov;

  const LanguageModel &model = LanguageModel::GetFirstLM();
  model.CalcScore(phrase, full, ignored_ngram, ignored_oov);
  // CalcScore transforms, but EvaluateChart doesn't.
  features.Assign(&model, full);
}

} // namespace Incremental
} // namespace Moses
