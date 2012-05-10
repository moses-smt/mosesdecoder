//
// Oliver Wilson <oliver.wilson@ed.ac.uk>
//

#include <Config.h>

#include "FactorCollection.h"
#include "LM/DMapLM.h"

namespace Moses
{

LanguageModelDMapLM::LanguageModelDMapLM() : m_lm(0) {
}

LanguageModelDMapLM::~LanguageModelDMapLM() {
  delete m_lm;
}

bool LanguageModelDMapLM::Load(const std::string& filePath,
                               FactorType factorType,
                               size_t nGramOrder)
{
  //std::cerr << "LanguageModelDMapLM: loading..." << std::endl;
  m_filePath = filePath;
  m_factorType = factorType;
  m_nGramOrder = nGramOrder;
  m_sentenceStart = FactorCollection::Instance().AddFactor(Output, factorType, "<s>");
  m_sentenceStartArray[m_factorType] = m_sentenceStart;
  m_sentenceEnd = FactorCollection::Instance().AddFactor(Output, factorType, "</s>");
  m_sentenceEndArray[m_factorType] = m_sentenceEnd;
  std::ifstream configFile(filePath.c_str());
  char struct_name_buffer[1024];
  char run_local_buffer[1024];
  configFile.getline(struct_name_buffer, 1024);
  configFile.getline(run_local_buffer, 1024);
  bool run_local;
  //std::cerr << "table name: " << struct_name_buffer << std::endl;
  //std::cerr << "run local: " << run_local_buffer << std::endl;
  if (strncmp(run_local_buffer, "true", 1024) == 0)
    run_local = true;
  else
    run_local = false;
  m_lm = new StructLanguageModelBackoff(Config::getConfig(), struct_name_buffer);
  return m_lm->init(run_local);
}

void LanguageModelDMapLM::CreateFactor(FactorCollection& factorCollection)
{
  // Don't know what this is for.
}

LMResult LanguageModelDMapLM::GetValueGivenState(
    const std::vector<const Word*>& contextFactor,
    FFState& state) const
{
  DMapLMState& cast_state = static_cast<DMapLMState&>(state);
  LMResult result;
  size_t succeeding_order;
  size_t target_order = std::min((size_t)cast_state.m_last_succeeding_order + 1,
                                 GetNGramOrder());
  result.score = GetValue(contextFactor, target_order, &succeeding_order);
  cast_state.m_last_succeeding_order = succeeding_order;
  return result;
}

LMResult LanguageModelDMapLM::GetValueForgotState(
    const std::vector<const Word*>& contextFactor,
    FFState& outState) const
{
  DMapLMState& cast_state = static_cast<DMapLMState&>(outState);
  LMResult result;
  size_t succeeding_order;
  size_t target_order = GetNGramOrder();
  result.score = GetValue(contextFactor, target_order, &succeeding_order);
  cast_state.m_last_succeeding_order = succeeding_order;
  return result;
}

float LanguageModelDMapLM::GetValue(
    const std::vector<const Word*>& contextFactor,
    size_t target_order,
    size_t* succeeding_order) const
{
  FactorType factorType = GetFactorType();
  float score;
  
  std::string ngram_string("");
  ngram_string.append(((*contextFactor[0])[factorType])->GetString());
  for (size_t i = 1; i < contextFactor.size(); ++i) {
    ngram_string.append(" ");
    ngram_string.append(((*contextFactor[i])[factorType])->GetString());
  }
  //std::cout << "ngram: X" << ngram_string << "X" << std::endl;
  score = m_lm->calcScore(ngram_string.c_str(), target_order, succeeding_order);
  score = FloorScore(TransformLMScore(score));
  return score;
}

const FFState* LanguageModelDMapLM::GetNullContextState() const {
    DMapLMState* state = new DMapLMState();
    state->m_last_succeeding_order = GetNGramOrder();
    return state;
}

FFState* LanguageModelDMapLM::GetNewSentenceState() const {
    DMapLMState* state = new DMapLMState();
    state->m_last_succeeding_order = GetNGramOrder();
    return state;
}

const FFState* LanguageModelDMapLM::GetBeginSentenceState() const {
    DMapLMState* state = new DMapLMState();
    state->m_last_succeeding_order = GetNGramOrder();
    return state;
}

FFState* LanguageModelDMapLM::NewState(const FFState* state) const {
    DMapLMState* new_state = new DMapLMState();
    const DMapLMState* cast_state = static_cast<const DMapLMState*>(state);
    new_state->m_last_succeeding_order = cast_state->m_last_succeeding_order;
    return new_state;
}

void LanguageModelDMapLM::CleanUpAfterSentenceProcessing() {
  m_lm->printStats();
  m_lm->resetStats();
  m_lm->clearCaches();
}

void LanguageModelDMapLM::InitializeBeforeSentenceProcessing() {
}

}  // namespace Moses

