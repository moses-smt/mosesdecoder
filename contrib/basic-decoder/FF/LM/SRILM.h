#pragma once

#include "LM.h"
#include <Vocab.h>

class Ngram;

namespace FastMoses
{

class SRILM : public LM
{
public:
  SRILM(const std::string &line);
  void Load();

  virtual size_t GetLastState() const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  Vocab 			*m_srilmVocab;
  Ngram 			*m_srilmModel;
  std::string m_path;
  VocabIndex	m_unknownId;
  std::vector<VocabIndex> m_lmIdLookup;
  mutable void *m_lastState;

  void CreateVocab();
  VocabIndex GetLmID( const std::string &str ) const;
  VocabIndex GetLmID(VOCABID vocabId) const;
  float GetValue(VocabIndex wordId, VocabIndex *context) const;

  virtual SCORE GetValue(const PhraseVec &phraseVec) const;

};

}
