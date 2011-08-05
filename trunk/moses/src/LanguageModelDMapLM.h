//
// Oliver Wilson <oliver.wilson@ed.ac.uk>
//

#ifndef moses_LanguageModelDMapLM_h
#define moses_LanguageModelDMapLM_h

#include <StructLanguageModelBackoff.h>

#include "Factor.h"
#include "LanguageModelSingleFactor.h"
#include "Util.h"

namespace Moses {

class LanguageModelDMapLM : public LanguageModelPointerState
{
public:
  LanguageModelDMapLM();
  ~LanguageModelDMapLM();
  bool Load(const std::string&, FactorType, size_t);
  virtual LMResult GetValue(const std::vector<const Word*>&, State*) const;
  void CleanUpAfterSentenceProcessing();
  void InitializeBeforeSentenceProcessing();

protected:
  StructLanguageModelBackoff* m_lm;

  void CreateFactor(FactorCollection&);

};

}  // namespace Moses

#endif  // moses_LanguageModelDMapLM_h

