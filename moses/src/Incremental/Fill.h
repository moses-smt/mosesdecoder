#pragma once

#include "ChartParserCallback.h"
#include "StackVec.h"

#include "lm/word_index.hh"
#include "search/edge_queue.hh"

#include <boost/pool/object_pool.hpp>

#include <list>
#include <vector>

namespace search {
template <class Model> class Context;
} // namespace search

namespace Moses {
class Word;
class WordsRange;
class TargetPhraseCollection;
class WordsRange;
class ChartCellLabelSet;
class TargetPhrase;

namespace Incremental {
class Owner;

// Replacement for ChartTranslationOptionList
// TODO: implement count and score thresholding.  
template <class Model> class Fill : public ChartParserCallback {
  public:
    Fill(search::Context<Model> &context, const std::vector<lm::WordIndex> &vocab_mapping, Owner &owner);

    void Add(const TargetPhraseCollection &targets, const StackVec &nts, const WordsRange &ignored);

    void AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection*> &waste_memory, const WordsRange &range);

    bool Empty() const { return edges_.Empty(); }

    void Search(ChartCellLabelSet &out);
    
  private:
    lm::WordIndex Convert(const Word &word) const ;

    search::Context<Model> &context_;

    const std::vector<lm::WordIndex> &vocab_mapping_;

    Owner &owner_;

    search::EdgeQueue edges_;
};

} // namespace Incremental
} // namespace Moses
