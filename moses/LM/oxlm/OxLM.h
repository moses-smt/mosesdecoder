// $Id$
#pragma once

#include <vector>

#include "moses/LM/SingleFactor.h"

// lbl stuff
#include "lbl/model.h"
#include "lbl/query_cache.h"

#include "OxLMMapper.h"

namespace Moses {

template<class Model>
class OxLM : public LanguageModelSingleFactor {
 public:
	OxLM(const std::string &line);

  ~OxLM();

  void SetParameter(const std::string& key, const std::string& value);

  void Load();

  double GetScore(int word, const std::vector<int>& context) const;

  virtual LMResult GetValue(
      const std::vector<const Word*> &contextFactor,
      State* finalState = 0) const;

  virtual void InitializeForInput(const InputType& source);

  virtual void CleanUpAfterSentenceProcessing(const InputType& source);

 protected:
  Model model;
  boost::shared_ptr<OxLMMapper> mapper;

  int kSTART;
  int kSTOP;
  int kUNKNOWN;

  bool normalized;

  bool persistentCache;
  mutable boost::thread_specific_ptr<oxlm::QueryCache> cache;
  mutable int cacheHits, totalHits;
};

} // namespace Moses
