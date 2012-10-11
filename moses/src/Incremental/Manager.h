#pragma once

#include "lm/word_index.hh"

#include "ChartCellCollection.h"
#include "ChartParser.h"
#include "Incremental/Owner.h"

namespace Moses {
class InputType;
class TranslationSystem;
namespace Incremental {

class Manager {
  public:
    Manager(const InputType &source, const TranslationSystem &system);

    ~Manager();

    template <class Model> void LMCallback(const Model &model, const std::vector<lm::WordIndex> &words);
    
    void ProcessSentence();

  private:
    const InputType &source_;
    const TranslationSystem &system_;
    ChartCellCollectionBase cells_;
    ChartParser parser_;
    Owner owner_;
};
} // namespace Incremental
} // namespace Moses

