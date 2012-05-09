#pragma once

#include <iosfwd>

class Alignment;
class SuffixArray;
class TargetCorpus;

class Mismatch
{
public:
  typedef unsigned int INDEX;

private:
  SuffixArray *m_suffixArray;
  TargetCorpus *m_targetCorpus;
  Alignment *m_alignment;
  INDEX m_sentence_id;
  INDEX m_num_alignment_points;
  char m_source_length;
  char m_target_length;
  INDEX m_source_position;
  char m_source_start;
  char m_source_end;
  bool m_source_unaligned[ 256 ];
  bool m_target_unaligned[ 256 ];
  bool m_unaligned;

  // No copying allowed.
  Mismatch(const Mismatch&);
  void operator=(const Mismatch&);

public:
  Mismatch( SuffixArray *sa, TargetCorpus *tc, Alignment *a, INDEX sentence_id, INDEX position, char source_length, char target_length, char source_start, char source_end );
  ~Mismatch();

  bool Unaligned() const { return m_unaligned; }
  void PrintClippedHTML(std::ostream* out, int width );
  void LabelSourceMatches( char *source_annotation, char *target_annotation, char source_id, char label );
};
