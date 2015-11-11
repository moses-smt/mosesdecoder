// -*- c++ -*-
#pragma once

#include "lm/word_index.hh"
#include "search/applied.hh"
#include "search/nbest.hh"

#include "moses/ChartCellCollection.h"
#include "moses/ChartParser.h"

#include "BaseManager.h"

#include <vector>
#include <string>

namespace Moses
{
class ScoreComponentCollection;
class InputType;
class LanguageModel;

namespace Incremental
{

class Manager : public BaseManager
{
public:
  Manager(ttasksptr const& ttask);

  ~Manager();

  template <class Model> void LMCallback(const Model &model, const std::vector<lm::WordIndex> &words);

  void Decode();

  const std::vector<search::Applied> &GetNBest() const;

  // Call to get the same value as ProcessSentence returned.
  const std::vector<search::Applied> &Completed() const {
    return *completed_nbest_;
  }

  // output
  void OutputBest(OutputCollector *collector) const;
  void OutputNBest(OutputCollector *collector) const;
  void OutputDetailedTranslationReport(OutputCollector *collector) const;
  void OutputNBestList(OutputCollector *collector, const std::vector<search::Applied> &nbest, long translationId) const;
  void OutputLatticeSamples(OutputCollector *collector) const {
  }
  void OutputAlignment(OutputCollector *collector) const {
  }
  void OutputDetailedTreeFragmentsTranslationReport(OutputCollector *collector) const;
  void OutputWordGraph(OutputCollector *collector) const {
  }
  void OutputSearchGraph(OutputCollector *collector) const {
  }
  void OutputSearchGraphSLF() const {
  }

  void
  OutputSearchGraphAsHypergraph
  ( std::string const& fname, size_t const precision ) const
  { }


private:
  template <class Model, class Best> search::History PopulateBest(const Model &model, const std::vector<lm::WordIndex> &words, Best &out);

  ChartCellCollectionBase cells_;
  ChartParser parser_;

  // Only one of single_best_ or n_best_ will be used, but it was easier to do this than a template.
  search::SingleBest single_best_;
  // ProcessSentence returns a reference to a vector.  ProcessSentence
  // doesn't have one, so this is populated and returned.
  std::vector<search::Applied> backing_for_single_;

  search::NBest n_best_;

  const std::vector<search::Applied> *completed_nbest_;

  // outputs
  void OutputDetailedTranslationReport(
    OutputCollector *collector,
    const search::Applied *applied,
    const Sentence &sentence,
    long translationId) const;
  void OutputTranslationOptions(std::ostream &out,
                                ApplicationContext &applicationContext,
                                const search::Applied *applied,
                                const Sentence &sentence,
                                long translationId) const;
  void OutputTranslationOption(std::ostream &out,
                               ApplicationContext &applicationContext,
                               const search::Applied *applied,
                               const Sentence &sentence,
                               long translationId) const;
  void ReconstructApplicationContext(const search::Applied *applied,
                                     const Sentence &sentence,
                                     ApplicationContext &context) const;
  void OutputTreeFragmentsTranslationOptions(std::ostream &out,
      ApplicationContext &applicationContext,
      const search::Applied *applied,
      const Sentence &sentence,
      long translationId) const;
  void OutputBestHypo(OutputCollector *collector, search::Applied applied, long translationId) const;
  void OutputBestNone(OutputCollector *collector, long translationId) const;

  void OutputUnknowns(OutputCollector *collector) const {
  }
  void CalcDecoderStatistics() const {
  }

};

// Just get the phrase.
void ToPhrase(const search::Applied final, Phrase &out);
// Get the phrase and the features.
void PhraseAndFeatures(const search::Applied final, Phrase &phrase, ScoreComponentCollection &features);


} // namespace Incremental
} // namespace Moses

