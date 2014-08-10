// $Id$
#pragma once

#include <vector>

#include "moses/LM/SingleFactor.h"

// lbl stuff
#include "corpus/corpus.h"
#include "lbl/model.h"
#include "lbl/query_cache.h"

#include "Mapper.h"

namespace Moses
{


template<class Model>
class LBLLM : public LanguageModelSingleFactor
{
public:
	LBLLM(const std::string &line);

  ~LBLLM();

  void SetParameter(const std::string& key, const std::string& value);

  void Load();

  virtual LMResult GetValue(
      const std::vector<const Word*> &contextFactor,
      State* finalState = 0) const;

  virtual void InitializeForInput(const InputType& source);

  virtual void CleanUpAfterSentenceProcessing(const InputType& source);

protected:
  Model model;
  boost::shared_ptr<OXLMMapper> mapper;

  int kSTART;
  int kSTOP;
  int kUNKNOWN;

  bool persistentCache;
  mutable boost::thread_specific_ptr<oxlm::QueryCache> cache;
  mutable int cacheHits, totalHits;
};


}
