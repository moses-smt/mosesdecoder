#pragma once

#include "ChartParserCallback.h"
#include "StackVec.h"
#include "Incremental/Owner.h"

namespace Moses {
namespace Incremental {

class TargetPhraseCollection;
class WordsRange;
class ChartCellLabelSet;

// Replacement for ChartTranslationOptionList
// TODO: implement count and score thresholding.  
class List : public ChartParserCallback {
  public:
    explicit List(ChartCellBase &fill) : fill_(fill) {}

    void Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordRange &ignored);

  private:
    ChartCellLabelSet &fill_;

    boost::object_pool<search::Vertex> &vertex_pool_;
    boost::object_pool<search::Edge> &edge_pool_;
};

} // namespace Incremental
} // namespace Moses
