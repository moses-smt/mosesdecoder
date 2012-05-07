#pragma once

#include "Vocabulary.h"

class TargetCorpus
{
public:
  typedef unsigned int INDEX;

private:
  WORD_ID *m_array;
  INDEX *m_sentenceEnd;
  Vocabulary m_vcb;
  INDEX m_size;
  INDEX m_sentenceCount;

public:
  ~TargetCorpus();

  void Create(const std::string& fileName );
  WORD GetWordFromId( const WORD_ID id ) const;
  WORD GetWord( INDEX sentence, char word );
  WORD_ID GetWordId( INDEX sentence, char word );
  char GetSentenceLength( INDEX sentence );
  void Load(const std::string& fileName );
  void Save(const std::string& fileName );
};
