#pragma once

#include <iosfwd>

class Alignment;
class SuffixArray;
class TargetCorpus;

class PhrasePair
{
public:
  typedef unsigned int INDEX;

private:
  SuffixArray *m_suffixArray;
  TargetCorpus *m_targetCorpus;
  Alignment *m_alignment;
  INDEX m_sentence_id;
  char m_target_length;
  INDEX m_source_position;
  char m_source_start, m_source_end;
  char m_target_start, m_target_end;
  char m_start_null, m_end_null;
  char m_pre_null, m_post_null;

public:
  PhrasePair( SuffixArray *sa, TargetCorpus *tc, Alignment *a, INDEX sentence_id, char target_length, INDEX position, char source_start, char source_end, char target_start, char target_end, char start_null, char end_null, char pre_null, char post_null)
    :m_suffixArray(sa)
    ,m_targetCorpus(tc)
    ,m_alignment(a)
    ,m_sentence_id(sentence_id)
    ,m_target_length(target_length)
    ,m_source_position(position)
    ,m_source_start(source_start)
    ,m_source_end(source_end)
    ,m_target_start(target_start)
    ,m_target_end(target_end)
    ,m_start_null(start_null)
    ,m_end_null(end_null)
    ,m_pre_null(pre_null)
    ,m_post_null(post_null) {
  }
  ~PhrasePair () {}

  void PrintTarget( std::ostream* out ) const;
  void Print( std::ostream* out ) const;
  void PrintPretty( std::ostream* out, int width ) const;
  void PrintHTML( std::ostream* out ) const;
  void PrintClippedHTML( std::ostream* out, int width ) const;
};
