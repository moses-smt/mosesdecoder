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

  // No copying allowed.
  TargetCorpus(const TargetCorpus&);
  void operator=(const TargetCorpus&);

public:
  TargetCorpus();
  ~TargetCorpus();

  void Create(const std::string& fileName );
  WORD GetWordFromId( const WORD_ID id ) const;
  WORD GetWord( INDEX sentence, int word ) const;
  WORD_ID GetWordId( INDEX sentence, int word ) const;
  char GetSentenceLength( INDEX sentence ) const;
  void Load(const std::string& fileName );
  void Save(const std::string& fileName ) const;
};
