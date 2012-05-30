#pragma once

#include <string>
#include <vector>
#include "Factor.h"
#include "Util.h"
#include "LM/SingleFactor.h"
#include "DynSAInclude/onlineRLM.h"
//#include "multiOnlineRLM.h"
#include "DynSAInclude/file.h"
#include "DynSAInclude/vocab.h"

namespace Moses
{
class Factor;
class Phrase;

class LanguageModelORLM : public LanguageModelPointerState {
public:
  typedef count_t T;  // type for ORLM filter
  LanguageModelORLM()
    : m_lm(0) {}
  bool Load(const std::string &filePath, FactorType factorType, size_t nGramOrder);
  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const;
  ~LanguageModelORLM() {
    //save LM with markings
    Utils::rtrim(m_filePath, ".gz");
    FileHandler fout(m_filePath + ".marked.gz", std::ios::out|std::ios::binary, false);
    m_lm->save(&fout);
    fout.close();
    delete m_lm;
  }
  void CleanUpAfterSentenceProcessing() {m_lm->clearCache();} // clear caches
  void InitializeBeforeSentenceProcessing() { // nothing to do
    //m_lm->initThreadSpecificData(); // Creates thread specific data iff
                                    // compiled with multithreading.
  }
  bool UpdateORLM(const std::vector<string>& ngram, const int value);
 protected:
  OnlineRLM<T>* m_lm;
  //MultiOnlineRLM<T>* m_lm;
  wordID_t m_oov_id;
  std::vector<wordID_t> lm_ids_vec_;
  void CreateFactors();
  wordID_t GetLmID(const std::string &str) const;
  wordID_t GetLmID(const Factor *factor) const;
};
} // end namespace

