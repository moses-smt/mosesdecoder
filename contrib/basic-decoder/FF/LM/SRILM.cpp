
#include <Ngram.h>
#include <Vocab.h>
#include "SRILM.h"
#include "MyVocab.h"
#include "Util.h"

using namespace std;

#define MAX_NGRAM_SIZE 10

namespace FastMoses
{

SRILM::SRILM(const string &line)
  :LM(line)
{
  ReadParameters();

}

void SRILM::Load()
{
  m_srilmVocab  = new Vocab();
  m_srilmModel	= new Ngram(*m_srilmVocab, m_order);

  m_srilmModel->skipOOVs() = false;

  File file(m_path.c_str(), "r" );
  m_srilmModel->read(file);

  CreateVocab();
  m_unknownId = m_srilmVocab->unkIndex();

}

void SRILM::CreateVocab()
{
  MyVocab &factorCollection = MyVocab::Instance();

  std::map<size_t, VocabIndex> lmIdMap;
  size_t maxFactorId = 0; // to create lookup vector later on

  VocabString str;
  VocabIter iter(*m_srilmVocab);
  while ( (str = iter.next()) != NULL) {
    VocabIndex lmId = GetLmID(str);
    VOCABID factorId = factorCollection.GetOrCreateId(str);
    lmIdMap[factorId] = lmId;
    maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  }

  VOCABID factorId;
  factorId = factorCollection.GetOrCreateId("<s>");
  lmIdMap[factorId] = GetLmID("<s>");
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;

  factorId = factorCollection.GetOrCreateId("</s>");
  lmIdMap[factorId] = GetLmID("</s>");
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;

  // add to lookup vector in object
  m_lmIdLookup.resize(maxFactorId+1);

  fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), m_unknownId);

  map<size_t, VocabIndex>::iterator iterMap;
  for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap) {
    m_lmIdLookup[iterMap->first] = iterMap->second;
  }

}

VocabIndex SRILM::GetLmID( const std::string &str ) const
{
  return m_srilmVocab->getIndex( str.c_str(), m_unknownId );
}

VocabIndex SRILM::GetLmID(VOCABID vocabId) const
{
  return ( vocabId >= m_lmIdLookup.size()) ? m_unknownId : m_lmIdLookup[vocabId];
}


SCORE SRILM::GetValue(const PhraseVec &phraseVec) const
{
  size_t count = phraseVec.size();
  if (count <= 0) {
    return 0;
  }

  // set up context
  VocabIndex context[MAX_NGRAM_SIZE];
  for (size_t i = 0 ; i < count - 1 ; i++) {
    const Word &word = *phraseVec[count-2-i];
    VOCABID vocabId = word.GetVocab();

    context[i] =  GetLmID(vocabId);
  }
  context[count-1] = Vocab_None;

  assert(phraseVec[count-1] != NULL);
  // call sri lm fn
  VocabIndex lmId= GetLmID(phraseVec[count-1]->GetVocab());
  float ret = GetValue(lmId, context);

  for (int i = count - 2 ; i >= 0 ; i--)
    context[i+1] = context[i];
  context[0] = lmId;
  unsigned len;
  m_lastState = m_srilmModel->contextID(context, len);
  len++;

  return ret;
}

float SRILM::GetValue(VocabIndex wordId, VocabIndex *context) const
{
  float p = m_srilmModel->wordProb( wordId, context );
  return FloorScore(TransformSRIScore(p));  // log10->log
}


size_t SRILM::GetLastState() const
{
  return (size_t)m_lastState;
}

void SRILM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else {
    LM::SetParameter(key, value);
  }
}

}

