#include "Incremental/Manager.h"

#include "Incremental/Edge.h"
#include "Incremental/Fill.h"

#include "ChartCell.h"
#include "TranslationSystem.h"
#include "StaticData.h"

#include "search/context.hh"
#include "search/config.hh"
#include "search/weights.hh"

namespace Moses {
namespace Incremental {

namespace {
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
  parser_(source, system, cells_) {

  system.InitializeBeforeSentenceProcessing(source);
}

Manager::~Manager() {
  system_.CleanUpAfterSentenceProcessing(source_);
}

namespace {

void ConstructString(const search::Final &final, std::ostringstream &stream) {
  const TargetPhrase &phrase = static_cast<const Edge&>(final.From()).GetMoses();
  size_t child = 0;
  for (std::size_t i = 0; i < phrase.GetSize(); ++i) {
    const Word &word = phrase.GetWord(i);
    if (word.IsNonTerminal()) {
      ConstructString(*final.Children()[child++], stream);
    } else {
      stream << word[0]->GetString() << ' ';
    }
  }
}

void BestString(const ChartCellLabelSet &labels, std::string &out) {
  const search::Final *best = NULL;
  for (ChartCellLabelSet::const_iterator i = labels.begin(); i != labels.end(); ++i) {
    const search::Final *child = i->second.GetStack().incr->BestChild();
    if (child && (!best || (child->Bound() > best->Bound()))) {
      best = child;
    }
  }
  if (!best) {
    out.clear();
    return;
  }
  std::ostringstream stream;
  ConstructString(*best, stream);
  out = stream.str();
  CHECK(out.size() > 9);
  // <s>
  out.erase(0, 4);
  // </s>
  out.erase(out.size() - 5);
}

} // namespace


template <class Model> void Manager::LMCallback(const Model &model, const std::vector<lm::WordIndex> &words) {
  const LanguageModel &abstract = **system_.GetLanguageModels().begin();
  search::Weights weights(
      abstract.GetWeight(), 
      abstract.OOVFeatureEnabled() ? abstract.GetOOVWeight() : 0.0, 
      system_.GetWeightWordPenalty());
  search::Config config(weights, StaticData::Instance().GetCubePruningPopLimit());
  search::Context<Model> context(config, model);

  size_t size = source_.GetSize();
  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);
      Fill<Model> filler(context, words, owner_);
      parser_.Create(range, filler);
      filler.Search(cells_.MutableBase(range).MutableTargetLabelSet());
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
