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
  int m_source_length;
  int m_target_length;
  INDEX m_source_position;
  int m_source_start;
  int m_source_end;
  bool m_source_unaligned[ 256 ];
  bool m_target_unaligned[ 256 ];
  bool m_unaligned;

  // No copying allowed.
  Mismatch(const Mismatch&);
  void operator=(const Mismatch&);

public:
  Mismatch( SuffixArray *sa, TargetCorpus *tc, Alignment *a, INDEX sentence_id, INDEX position, int source_length, int target_length, int source_start, int source_end );
  ~Mismatch();

  bool Unaligned() const {
    return m_unaligned;
  }
  void PrintClippedHTML(std::ostream* out, int width );
  void LabelSourceMatches(int *source_annotation, int *target_annotation, int source_id, int label );
};
