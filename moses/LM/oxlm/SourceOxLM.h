#pragma once

#include <vector>

#include "lbl/model.h"
#include "lbl/query_cache.h"

#include "moses/LM/BilingualLM.h"
#include "moses/LM/oxlm/OxLMParallelMapper.h"

namespace Moses
{

class SourceOxLM : public BilingualLM
{
public:
  SourceOxLM(const std::string &line);

  ~SourceOxLM();

private:
  virtual float Score(
    std::vector<int>& source_words,
    std::vector<int>& target_words) const;

  virtual int getNeuralLMId(const Word& word, bool is_source_word) const;

  virtual void loadModel();

  const Word& getNullWord() const;

  void SetParameter(const std::string& key, const std::string& value);

  void InitializeForInput(ttasksptr const& ttask);

  void CleanUpAfterSentenceProcessing(const InputType& source);

protected:
  oxlm::SourceFactoredLM model;
  boost::shared_ptr<OxLMParallelMapper> mapper;

  bool posBackOff;
  FactorType posFactorType;

  bool persistentCache;
  mutable boost::thread_specific_ptr<oxlm::QueryCache> cache;
  mutable int cacheHits, totalHits;
  Word NULL_word; //Null symbol for hiero
};

} // namespace Moses
