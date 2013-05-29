#include "Vocabulary.h"

#pragma once

#define LINE_MAX_LENGTH 10000

namespace tmmt
{

class SuffixArray
{
public:
  typedef unsigned int INDEX;

private:
  std::vector< std::vector< WORD_ID > > corpus;

  WORD_ID *m_array;
  INDEX *m_index;
  INDEX *m_buffer;
  char *m_wordInSentence;
  size_t *m_sentence;
  char *m_sentenceLength;
  WORD_ID m_endOfSentence;
  Vocabulary m_vcb;
  INDEX m_size;

public:
  SuffixArray( std::string fileName );
  ~SuffixArray();

  void Sort(INDEX start, INDEX end);
  int CompareIndex( INDEX a, INDEX b ) const;
  inline int CompareWord( WORD_ID a, WORD_ID b ) const;
  int Count( const std::vector< WORD > &phrase );
  bool MinCount( const std::vector< WORD > &phrase, INDEX min );
  bool Exists( const std::vector< WORD > &phrase );
  int FindMatches( const std::vector< WORD > &phrase, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start = 0, INDEX search_end = -1 );
  int LimitedCount( const std::vector< WORD > &phrase, INDEX min, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start = -1, INDEX search_end = 0 );
  INDEX FindFirst( const std::vector< WORD > &phrase, INDEX &start, INDEX &end );
  INDEX FindLast( const std::vector< WORD > &phrase, INDEX start, INDEX end, int direction );
  int Match( const std::vector< WORD > &phrase, INDEX index );
  void List( INDEX start, INDEX end );
  inline INDEX GetPosition( INDEX index ) {
    return m_index[ index ];
  }
  inline size_t GetSentence( INDEX position ) {
    return m_sentence[position];
  }
  inline char GetWordInSentence( INDEX position ) {
    return m_wordInSentence[position];
  }
  inline char GetSentenceLength( size_t sentenceId ) {
    return m_sentenceLength[sentenceId];
  }
  inline INDEX GetSize() {
    return m_size;
  }

  Vocabulary &GetVocabulary() {
    return m_vcb;
  }
  const std::vector< std::vector< WORD_ID > > &GetCorpus() const {
    return corpus;
  }
};

}

