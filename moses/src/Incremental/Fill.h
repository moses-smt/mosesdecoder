#pragma once

#include "ChartParserCallback.h"
#include "StackVec.h"
#include "Incremental/Owner.h"

#include "search/context.hh"
#include "search/edge_queue.hh"

namespace Moses {
namespace Incremental {

class TargetPhraseCollection;
class WordsRange;
class ChartCellLabelSet;

// Replacement for ChartTranslationOptionList
// TODO: implement count and score thresholding.  
template <class Model> class Fill : public ChartParserCallback {
  public:
    void Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordRange &ignored);

    void Search();
    
  private:
    search::Context<Model> &context_;

    search::EdgeQueue edges_;

    boost::object_pool<search::Edge> &edge_pool_;
};

} // namespace Incremental
} // namespace Moses
