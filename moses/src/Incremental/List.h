#pragma once

namespace Moses {
namespace Incremental {

// Replacement for ChartTranslationOptionList
class List {
  public:
    List();

    void Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordRange &ignored);

    void Process(ChartCellLabelSet &out);

    void ShrinkToLimit() const {}
    void ApplyThreshold() const {}

  private:
    size_t limit_;

    float threshold_;

    search::Vertex  
};

} // namespace Incremental
} // namespace Moses
