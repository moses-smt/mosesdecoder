#include <cmath>
#include <stdexcept>

#include "moses/Incremental.h"

#include "moses/ChartCell.h"
#include "moses/ChartParserCallback.h"
#include "moses/FeatureVector.h"
#include "moses/StaticData.h"
#include "moses/Util.h"
#include "moses/LM/Base.h"
#include "moses/OutputCollector.h"

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
      entry = generator_pool_.construct(boost::ref(context_), boost::ref(*vertex_pool_.construct()), boost::ref(best_));
      stack.incr_generator = entry;
    }
    entry->NewHypothesis(partial);
  }

  void FinishedSearch() {
    for (ChartCellLabelSet::iterator i(out_.mutable_begin()); i != out_.mutable_end(); ++i) {
      if ((*i) == NULL) {
        continue;
      }
      ChartCellLabel::Stack &stack = (*i)->MutableStack();
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

  void Add(const TargetPhraseCollection &targets, const StackVec &nts, const Range &ignored);

  void AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection::shared_ptr > &waste_memory, const Range &range);

  float GetBestScore(const ChartCellLabel *chartCell) const;

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

  void EvaluateWithSourceContext(const InputType &input, const InputPath &inputPath) {
    // TODO for input lattice
  }
private:
  lm::WordIndex Convert(const Word &word) const;

  search::Context<Model> &context_;

  const std::vector<lm::WordIndex> &vocab_mapping_;

  search::EdgeGenerator edges_;

  const search::Score oov_weight_;
};

template <class Model> void Fill<Model>::Add(const TargetPhraseCollection &targets, const StackVec &nts, const Range &range)
{
  std::vector<search::PartialVertex> vertices;
  vertices.reserve(nts.size());
  float below_score = 0.0;
  for (StackVec::const_iterator i(nts.begin()); i != nts.end(); ++i) {
    vertices.push_back((*i)->GetStack().incr->RootAlternate());
    below_score += (*i)->GetBestScore(this);
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
    edge.SetRange(range);

    edges_.AddEdge(edge);
  }
}

template <class Model> void Fill<Model>::AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection::shared_ptr > &, const Range &range)
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
  edge.SetRange(range);

  edges_.AddEdge(edge);
}

// for pruning
template <class Model> float Fill<Model>::GetBestScore(const ChartCellLabel *chartCell) const
{
  search::PartialVertex vertex = chartCell->GetStack().incr->RootAlternate();
  UTIL_THROW_IF2(vertex.Empty(), "hypothesis with empty stack");
  return vertex.Bound();
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

Manager::Manager(ttasksptr const& ttask)
  : BaseManager(ttask)
  , cells_(m_source, ChartCellBaseFactory(), parser_)
  , parser_(ttask, cells_)
  , n_best_(search::NBestConfig(StaticData::Instance().options()->nbest.nbest_size))
{ }

Manager::~Manager()
{ }


namespace
{
// Natural logarithm of 10.
//
// Some implementations of <cmath> define M_LM10, but not all.
const float log_10 = logf(10);
}

template <class Model, class Best>
search::History
Manager::
PopulateBest(const Model &model, const std::vector<lm::WordIndex> &words, Best &out)
{
  const LanguageModel &abstract = LanguageModel::GetFirstLM();
  const StaticData &data = StaticData::Instance();
  const float lm_weight = data.GetWeights(&abstract)[0];
  const float oov_weight = abstract.OOVFeatureEnabled() ? data.GetWeights(&abstract)[1] : 0.0;
  size_t cpl = data.options()->cube.pop_limit;
  size_t nbs = data.options()->nbest.nbest_size;
  search::Config config(lm_weight * log_10, cpl, search::NBestConfig(nbs));
  search::Context<Model> context(config, model);

  size_t size = m_source.GetSize();
  boost::object_pool<search::Vertex> vertex_pool(std::max<size_t>(size * size / 2, 32));

  for (int startPos = size-1; startPos >= 0; --startPos) {
    for (size_t width = 1; width <= size-startPos; ++width) {
      // full range uses RootSearch
      if (startPos == 0 && startPos + width == size) {
        break;
      }
      Range range(startPos, startPos + width - 1);
      Fill<Model> filler(context, words, oov_weight);
      parser_.Create(range, filler);
      filler.Search(out, cells_.MutableBase(range).MutableTargetLabelSet(), vertex_pool);
    }
  }

  Range range(0, size - 1);
  Fill<Model> filler(context, words, oov_weight);
  parser_.Create(range, filler);
  return filler.RootSearch(out);
}

template <class Model> void Manager::LMCallback(const Model &model, const std::vector<lm::WordIndex> &words)
{
  std::size_t nbest = StaticData::Instance().options()->nbest.nbest_size;
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

void Manager::Decode()
{
  LanguageModel::GetFirstLM().IncrementalCallback(*this);
}

const std::vector<search::Applied> &Manager::GetNBest() const
{
  return *completed_nbest_;
}

void Manager::OutputBest(OutputCollector *collector) const
{
  const long translationId = m_source.GetTranslationId();
  const std::vector<search::Applied> &nbest = GetNBest();
  if (!nbest.empty()) {
    OutputBestHypo(collector, nbest[0], translationId);
  } else {
    OutputBestNone(collector, translationId);
  }

}


void Manager::OutputNBest(OutputCollector *collector)  const
{
  if (collector == NULL) {
    return;
  }

  OutputNBestList(collector, *completed_nbest_, m_source.GetTranslationId());
}

void
Manager::
OutputNBestList(OutputCollector *collector,
                std::vector<search::Applied> const& nbest,
                long translationId) const
{
  const std::vector<Moses::FactorType> &outputFactorOrder
  = options()->output.factor_order;

  std::ostringstream out;
  // wtf? copied from the original OutputNBestList
  if (collector->OutputIsCout()) {
    FixPrecision(out);
  }
  Phrase outputPhrase;
  ScoreComponentCollection features;
  for (std::vector<search::Applied>::const_iterator i = nbest.begin();
       i != nbest.end(); ++i) {
    Incremental::PhraseAndFeatures(*i, outputPhrase, features);
    // <s> and </s>
    UTIL_THROW_IF2(outputPhrase.GetSize() < 2,
                   "Output phrase should have contained at least 2 words "
                   << "(beginning and end-of-sentence)");

    outputPhrase.RemoveWord(0);
    outputPhrase.RemoveWord(outputPhrase.GetSize() - 1);
    out << translationId << " ||| ";
    OutputSurface(out, outputPhrase); // , outputFactorOrder, false);
    out << " ||| ";
    bool with_labels = options()->nbest.include_feature_labels;
    features.OutputAllFeatureScores(out, with_labels);
    out << " ||| " << i->GetScore() << '\n';
  }
  out << std::flush;
  assert(collector);
  collector->Write(translationId, out.str());
}

void
Manager::
OutputDetailedTranslationReport(OutputCollector *collector) const
{
  if (collector && !completed_nbest_->empty()) {
    const search::Applied &applied = completed_nbest_->at(0);
    OutputDetailedTranslationReport(collector,
                                    &applied,
                                    static_cast<const Sentence&>(m_source),
                                    m_source.GetTranslationId());
  }

}

void Manager::OutputDetailedTranslationReport(
  OutputCollector *collector,
  const search::Applied *applied,
  const Sentence &sentence,
  long translationId) const
{
  if (applied == NULL) {
    return;
  }
  std::ostringstream out;
  ApplicationContext applicationContext;

  OutputTranslationOptions(out, applicationContext, applied, sentence, translationId);
  collector->Write(translationId, out.str());
}

void Manager::OutputTranslationOptions(std::ostream &out,
                                       ApplicationContext &applicationContext,
                                       const search::Applied *applied,
                                       const Sentence &sentence, long translationId) const
{
  if (applied != NULL) {
    OutputTranslationOption(out, applicationContext, applied, sentence, translationId);
    out << std::endl;
  }

  // recursive
  const search::Applied *child = applied->Children();
  for (size_t i = 0; i < applied->GetArity(); i++) {
    OutputTranslationOptions(out, applicationContext, child++, sentence, translationId);
  }
}

void Manager::OutputTranslationOption(std::ostream &out,
                                      ApplicationContext &applicationContext,
                                      const search::Applied *applied,
                                      const Sentence &sentence,
                                      long translationId) const
{
  ReconstructApplicationContext(applied, sentence, applicationContext);
  const TargetPhrase &phrase = *static_cast<const TargetPhrase*>(applied->GetNote().vp);
  out << "Trans Opt " << translationId
      << " " << applied->GetRange()
      << ": ";
  WriteApplicationContext(out, applicationContext);
  out << ": " << phrase.GetTargetLHS()
      << "->" << phrase
      << " " << applied->GetScore(); // << hypo->GetScoreBreakdown() TODO: missing in incremental search hypothesis
}

// Given a hypothesis and sentence, reconstructs the 'application context' --
// the source RHS symbols of the SCFG rule that was applied, plus their spans.
void Manager::ReconstructApplicationContext(const search::Applied *applied,
    const Sentence &sentence,
    ApplicationContext &context) const
{
  context.clear();
  const Range &span = applied->GetRange();
  const search::Applied *child = applied->Children();
  size_t i = span.GetStartPos();
  size_t j = 0;

  while (i <= span.GetEndPos()) {
    if (j == applied->GetArity() || i < child->GetRange().GetStartPos()) {
      // Symbol is a terminal.
      const Word &symbol = sentence.GetWord(i);
      context.push_back(std::make_pair(symbol, Range(i, i)));
      ++i;
    } else {
      // Symbol is a non-terminal.
      const Word &symbol = static_cast<const TargetPhrase*>(child->GetNote().vp)->GetTargetLHS();
      const Range &range = child->GetRange();
      context.push_back(std::make_pair(symbol, range));
      i = range.GetEndPos()+1;
      ++child;
      ++j;
    }
  }
}

void Manager::OutputDetailedTreeFragmentsTranslationReport(OutputCollector *collector) const
{
  if (collector == NULL || Completed().empty()) {
    return;
  }

  const search::Applied *applied = &Completed()[0];
  const Sentence &sentence = static_cast<const Sentence &>(m_source);
  const size_t translationId = m_source.GetTranslationId();

  std::ostringstream out;
  ApplicationContext applicationContext;

  OutputTreeFragmentsTranslationOptions(out, applicationContext, applied, sentence, translationId);

  //Tree of full sentence
  //TODO: incremental search doesn't support stateful features

  collector->Write(translationId, out.str());

}

void Manager::OutputTreeFragmentsTranslationOptions(std::ostream &out,
    ApplicationContext &applicationContext,
    const search::Applied *applied,
    const Sentence &sentence,
    long translationId) const
{

  if (applied != NULL) {
    OutputTranslationOption(out, applicationContext, applied, sentence, translationId);

    const TargetPhrase &currTarPhr = *static_cast<const TargetPhrase*>(applied->GetNote().vp);

    out << " ||| ";
    if (const PhraseProperty *property = currTarPhr.GetProperty("Tree")) {
      out << " " << *property->GetValueString();
    } else {
      out << " " << "noTreeInfo";
    }
    out << std::endl;
  }

  // recursive
  const search::Applied *child = applied->Children();
  for (size_t i = 0; i < applied->GetArity(); i++) {
    OutputTreeFragmentsTranslationOptions(out, applicationContext, child++, sentence, translationId);
  }
}

void Manager::OutputBestHypo(OutputCollector *collector, search::Applied applied, long translationId) const
{
  if (collector == NULL) return;
  std::ostringstream out;
  FixPrecision(out);
  if (options()->output.ReportHypoScore) {
    out << applied.GetScore() << ' ';
  }
  Phrase outPhrase;
  Incremental::ToPhrase(applied, outPhrase);
  // delete 1st & last
  UTIL_THROW_IF2(outPhrase.GetSize() < 2,
                 "Output phrase should have contained at least 2 words (beginning and end-of-sentence)");
  outPhrase.RemoveWord(0);
  outPhrase.RemoveWord(outPhrase.GetSize() - 1);
  out << outPhrase.GetStringRep(options()->output.factor_order);
  out << '\n';
  collector->Write(translationId, out.str());

  VERBOSE(1,"BEST TRANSLATION: " << outPhrase << "[total=" << applied.GetScore() << "]" << std::endl);
}

void
Manager::
OutputBestNone(OutputCollector *collector, long translationId) const
{
  if (collector == NULL) return;
  if (options()->output.ReportHypoScore) {
    collector->Write(translationId, "0 \n");
  } else {
    collector->Write(translationId, "\n");
  }
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
  // CalcScore transforms, but EvaluateWhenApplied doesn't.
  features.Assign(&model, full);
}

} // namespace Incremental
} // namespace Moses
