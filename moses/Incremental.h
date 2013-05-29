#pragma once

#include "lm/word_index.hh"
#include "search/applied.hh"
#include "search/nbest.hh"

#include "moses/ChartCellCollection.h"
#include "moses/ChartParser.h"

#include <vector>
#include <string>

namespace Moses
{
class ScoreComponentCollection;
class InputType;
class LanguageModel;

namespace Incremental
{

class Manager
{
public:
  Manager(const InputType &source);

  ~Manager();

  template <class Model> void LMCallback(const Model &model, const std::vector<lm::WordIndex> &words);

  const std::vector<search::Applied> &ProcessSentence();

  // Call to get the same value as ProcessSentence returned.
  const std::vector<search::Applied> &Completed() const {
    return *completed_nbest_;
  }

private:
  template <class Model, class Best> search::History PopulateBest(const Model &model, const std::vector<lm::WordIndex> &words, Best &out);

  const InputType &source_;
  ChartCellCollectionBase cells_;
  ChartParser parser_;

  // Only one of single_best_ or n_best_ will be used, but it was easier to do this than a template.
  search::SingleBest single_best_;
  // ProcessSentence returns a reference to a vector.  ProcessSentence
  // doesn't have one, so this is populated and returned.
  std::vector<search::Applied> backing_for_single_;

  search::NBest n_best_;

  const std::vector<search::Applied> *completed_nbest_;
};

// Just get the phrase.
void ToPhrase(const search::Applied final, Phrase &out);
// Get the phrase and the features.
void PhraseAndFeatures(const search::Applied final, Phrase &phrase, ScoreComponentCollection &features);


} // namespace Incremental
} // namespace Moses

