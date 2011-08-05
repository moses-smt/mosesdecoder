#include "Vocabulary.h"

#pragma once

#define LINE_MAX_LENGTH 10000


class SuffixArray
{
public:
  typedef unsigned int INDEX;

private:
  WORD_ID *m_array;
  INDEX *m_index;
  INDEX *m_buffer;
  char *m_wordInSentence;
  INDEX *m_sentence;
  char *m_sentenceLength;
  WORD_ID m_endOfSentence;
  Vocabulary m_vcb;
  INDEX m_size;
  INDEX m_sentenceCount;

public:
  ~SuffixArray();

  void Create( string fileName );
  void Sort(INDEX start, INDEX end);
  int CompareIndex( INDEX a, INDEX b ) const;
  inline int CompareWord( WORD_ID a, WORD_ID b ) const;
  int Count( const vector< WORD > &phrase );
  bool MinCount( const vector< WORD > &phrase, INDEX min );
  bool Exists( const vector< WORD > &phrase );
  int FindMatches( const vector< WORD > &phrase, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start = 0, INDEX search_end = -1 );
  int LimitedCount( const vector< WORD > &phrase, INDEX min, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start = -1, INDEX search_end = 0 );
  INDEX FindFirst( const vector< WORD > &phrase, INDEX &start, INDEX &end );
  INDEX FindLast( const vector< WORD > &phrase, INDEX start, INDEX end, int direction );
  int Match( const vector< WORD > &phrase, INDEX index );
  void List( INDEX start, INDEX end );
  inline INDEX GetPosition( INDEX index ) {
    return m_index[ index ];
  }
  inline INDEX GetSentence( INDEX position ) {
    return m_sentence[position];
  }
  inline char GetWordInSentence( INDEX position ) {
    return m_wordInSentence[position];
  }
  inline char GetSentenceLength( INDEX sentenceId ) {
    return m_sentenceLength[sentenceId];
  }
  inline INDEX GetSize() {
    return m_size;
  }
  inline WORD GetWord( INDEX position ) {
    return m_vcb.GetWord( m_array[position] );
  }
  void Save( string fileName );
  void Load( string fileName );
};
