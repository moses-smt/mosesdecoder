#include "Incremental/Manager.h"

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

template <class Model> void Manager::LMCallback(const Model &model, const std::vector<lm::WordIndex> &words) {
  const LanguageModel &abstract = **system_.GetLanguageModels().begin();
  search::Weights weights(
      abstract.GetWeight(), 
      abstract.OOVFeatureEnabled() ? abstract.GetOOVWeight() : 0.0, 
      system_.GetWeightWordPenalty());
  search::Config config(weights, StaticData::GetCubePruningPopLimit());
  search::Context<Model> context(config, model);

  size_t size = source_.GetSize();
  for (size_t width = 1; width <= size; ++width) {
    for (size_t startPos = 0; startPos <= size-width; ++startPos) {
      size_t endPos = startPos + width - 1;
      WordsRange range(startPos, endPos);
      Fill<Model> filler(context, words, owner_);
      parser_.Create(WordsRange(startPos, endPos), filler);
      filler.Search(cells_.MutableBase(range).MutableTargetLabelSet());
    }
  }
}

void Manager::ProcessSentence() {
  const LMList &lms = system_.GetLanguageModels();
  UTIL_THROW_IF(lms.size() != 1, util::Exception, "Incremental search only supports one language model.");
  (*lms.begin())->IncrementalCallback(*this);
}

} // namespace Incremental
} // namespace Moses
