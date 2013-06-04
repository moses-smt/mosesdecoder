#include <limits>
#include <iostream>
#include <fstream>

#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "ORLM.h"

using std::map;
namespace Moses
{
bool LanguageModelORLM::Load(const std::string &filePath, FactorType factorType,
                             size_t nGramOrder)
{
  cerr << "Loading LanguageModelORLM..." << endl;
  m_filePath = filePath;
  m_factorType = factorType;
  m_nGramOrder = nGramOrder;
  FileHandler fLmIn(m_filePath, std::ios::in|std::ios::binary, true);
  m_lm = new OnlineRLM<T>(&fLmIn, m_nGramOrder);
  fLmIn.close();
  //m_lm = new MultiOnlineRLM<T>(m_filePath, m_nGramOrder);
  // get special word ids
  m_oov_id = m_lm->vocab_->GetWordID("<unk>");
  CreateFactors();
  return true;
}
void LanguageModelORLM::CreateFactors()
{
  FactorCollection &factorCollection = FactorCollection::Instance();
  size_t maxFactorId = 0; // to create lookup vector later on
  std::map<size_t, wordID_t> m_lmids_map; // map from factor id -> word id

  for(std::map<Word, wordID_t>::const_iterator vIter = m_lm->vocab_->VocabStart();
      vIter != m_lm->vocab_->VocabEnd(); vIter++) {
    // get word from ORLM vocab and associate with (new) factor id
    size_t factorId = factorCollection.AddFactor(Output,m_factorType,vIter->first.ToString())->GetId();
    m_lmids_map[factorId] = vIter->second;
    maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  }
  // add factors for BOS and EOS and store bf word ids
  size_t factorId;
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, "<s>");
  factorId = m_sentenceStart->GetId();
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceStartWord[m_factorType] = m_sentenceStart;

  m_sentenceEnd	= factorCollection.AddFactor(Output, m_factorType, "</s>");
  factorId = m_sentenceEnd->GetId();
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;
  // add to lookup vector in object
  lm_ids_vec_.resize(maxFactorId+1);
  // fill with OOV code
  fill(lm_ids_vec_.begin(), lm_ids_vec_.end(), m_oov_id);

  for (map<size_t, wordID_t>::const_iterator iter = m_lmids_map.begin();
       iter != m_lmids_map.end() ; ++iter)
    lm_ids_vec_[iter->first] = iter->second;
}
wordID_t LanguageModelORLM::GetLmID(const std::string& str) const
{
  return m_lm->vocab_->GetWordID(str);
}
wordID_t LanguageModelORLM::GetLmID(const Factor* factor) const
{
  size_t factorId = factor->GetId();
  return (factorId >= lm_ids_vec_.size()) ? m_oov_id : lm_ids_vec_[factorId];
}
LMResult LanguageModelORLM::GetValue(const std::vector<const Word*> &contextFactor,
                                     State* finalState) const
{
  FactorType factorType = GetFactorType();
  // set up context
  //std::vector<long unsigned int> factor(1,0);
  //std::vector<string> sngram;
  wordID_t ngram[MAX_NGRAM_SIZE];
  int count = contextFactor.size();
  for (int i = 0; i < count; i++) {
    ngram[i] = GetLmID((*contextFactor[i])[factorType]);
    //sngram.push_back(contextFactor[i]->GetString(factor, false));
  }
  //float logprob = FloorScore(TransformLMScore(lm_->getProb(sngram, count, finalState)));
  LMResult ret;
  ret.score = FloorScore(TransformLMScore(m_lm->getProb(&ngram[0], count, finalState)));
  ret.unknown = count && (ngram[count - 1] == m_oov_id);
  /*if (finalState)
    std::cout << " = " << logprob << "(" << *finalState << ", " << *len <<")"<< std::endl;
  else
    std::cout << " = " << logprob << std::endl;
  */
  return ret;
}
bool LanguageModelORLM::UpdateORLM(const std::vector<string>& ngram, const int value)
{
  /*cerr << "Inserting into ORLM: \"";
  iterate(ngram, nit)
    cerr << *nit << " ";
  cerr << "\"\t" << value << endl; */
  m_lm->vocab_->MakeOpen();
  bool res = m_lm->update(ngram, value);
  m_lm->vocab_->MakeClosed();
  return res;
}
}
