#pragma once

#include "Vocabulary.h"

class Alignment
{
public:
  typedef unsigned int INDEX;

private:
  int *m_array;
  INDEX *m_sentenceEnd;
  INDEX m_size;
  INDEX m_sentenceCount;
  char m_unaligned[ 256 ]; // here for speed (local to PhraseAlignment)

  // No copying allowed.
  Alignment(const Alignment&);
  void operator=(const Alignment&);

public:
  Alignment();
  ~Alignment();

  void Create(const std::string& fileName );
  bool PhraseAlignment( INDEX sentence, int target_length,
                        int source_start, int source_end,
                        int &target_start, int &target_end,
                        int &pre_null, int &post_null );
  void Load(const std::string& fileName );
  void Save(const std::string& fileName ) const;
  std::vector<std::string> Tokenize( const char input[] );

  INDEX GetSentenceStart( INDEX sentence ) const {
    if (sentence == 0) return 0;
    return m_sentenceEnd[ sentence-1 ] + 2;
  }
  INDEX GetNumberOfAlignmentPoints( INDEX sentence ) const {
    return ( m_sentenceEnd[ sentence ] - GetSentenceStart( sentence ) ) / 2;
  }
  int GetSourceWord( INDEX sentence, INDEX alignment_point ) const {
    return m_array[ GetSentenceStart( sentence ) + alignment_point*2 ];
  }
  int GetTargetWord( INDEX sentence, INDEX alignment_point ) const {
    return m_array[ GetSentenceStart( sentence ) + alignment_point*2 + 1 ];
  }
};
