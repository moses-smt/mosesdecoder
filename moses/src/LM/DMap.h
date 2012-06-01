//
// Oliver Wilson <oliver.wilson@ed.ac.uk>
//

#ifndef moses_LanguageModelDMapLM_h
#define moses_LanguageModelDMapLM_h

#include <StructLanguageModelBackoff.h>

#include "Factor.h"
#include "FFState.h"
#include "LM/SingleFactor.h"
#include "Util.h"

namespace Moses {

class DMapLMState : public FFState {
public:
    int Compare(const FFState &o) const {
        const DMapLMState& cast_other = static_cast<const DMapLMState&>(o);
        if (cast_other.m_last_succeeding_order < m_last_succeeding_order)
            return -1;
        else if (cast_other.m_last_succeeding_order > m_last_succeeding_order)
            return 1;
        else
            return 0;
    }
    uint8_t m_last_succeeding_order;
};

class LanguageModelDMapLM : public LanguageModelSingleFactor
{
public:
  LanguageModelDMapLM();
  ~LanguageModelDMapLM();
  bool Load(const std::string&, FactorType, size_t);
  LMResult GetValueGivenState(const std::vector<const Word*>&, FFState&) const;
  LMResult GetValueForgotState(const std::vector<const Word*>&, FFState&) const;
  float GetValue(const std::vector<const Word*>&, size_t, size_t*) const;
  const FFState* GetNullContextState() const;
  FFState* GetNewSentenceState() const;
  const FFState* GetBeginSentenceState() const;
  FFState* NewState(const FFState*) const;
  void CleanUpAfterSentenceProcessing();
  void InitializeBeforeSentenceProcessing();

protected:
  StructLanguageModelBackoff* m_lm;

  void CreateFactor(FactorCollection&);

};

}  // namespace Moses

#endif  // moses_LanguageModelDMapLM_h

