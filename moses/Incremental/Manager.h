#pragma once

#include "lm/word_index.hh"

#include "moses/ChartCellCollection.h"
#include "moses/ChartParser.h"

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

    const std::string &String() const { return output_; }

  private:
    const InputType &source_;
    const TranslationSystem &system_;
    ChartCellCollectionBase cells_;
    ChartParser parser_;

    std::string output_;
};
} // namespace Incremental
} // namespace Moses

