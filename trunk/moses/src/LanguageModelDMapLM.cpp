//
// Oliver Wilson <oliver.wilson@ed.ac.uk>
//

#include <Config.h>

#include "FactorCollection.h"
#include "LanguageModelDMapLM.h"

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

LMResult LanguageModelDMapLM::GetValue(
    const std::vector<const Word*>& contextFactor,
    State* finalState) const
{
  FactorType factorType = GetFactorType();
  LMResult result;
  
  std::string ngram_string("");
  ngram_string.append(((*contextFactor[0])[factorType])->GetString());
  for (size_t i = 1; i < contextFactor.size(); ++i) {
    ngram_string.append(" ");
    ngram_string.append(((*contextFactor[i])[factorType])->GetString());
  }
  //std::cout << "ngram: X" << ngram_string << "X" << std::endl;
  result.score = FloorScore(TransformLMScore(m_lm->calcScore(ngram_string.c_str())));
  return result;
}

void LanguageModelDMapLM::CleanUpAfterSentenceProcessing() {
  m_lm->printStats();
  m_lm->resetStats();
}

void LanguageModelDMapLM::InitializeBeforeSentenceProcessing() {
}

}  // namespace Moses

