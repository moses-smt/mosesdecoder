#include <string>
#include <stdlib.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include "SuffixArray.h"
#include "TargetCorpus.h"
#include "Alignment.h"
#pragma once

using namespace std;

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
  SuffixArray::INDEX m_source_position;
  char m_source_start, m_source_end;
	char m_source_unaligned[ 256 ];
	char m_target_unaligned[ 256 ];
	char m_unaligned;

public:
  Mismatch( SuffixArray *sa, TargetCorpus *tc, Alignment *a, INDEX sentence_id, INDEX position, char source_length, char target_length, char source_start, char source_end )
    :m_suffixArray(sa)
    ,m_targetCorpus(tc)
    ,m_alignment(a)
    ,m_sentence_id(sentence_id)
    ,m_source_position(position)
		,m_source_length(source_length)
    ,m_target_length(target_length)
    ,m_source_start(source_start)
    ,m_source_end(source_end)
  {
		// initialize unaligned indexes
		for(char i=0; i<m_source_length; i++) {
			m_source_unaligned[i] = true;
		}
		for(char i=0; i<m_target_length; i++) {
			m_target_unaligned[i] = true;
		}
		m_num_alignment_points = 
			m_alignment->GetNumberOfAlignmentPoints( sentence_id );
		for(INDEX ap=0; ap<m_num_alignment_points; ap++) {
			m_source_unaligned[ m_alignment->GetSourceWord( sentence_id, ap ) ] = false;
			m_target_unaligned[ m_alignment->GetTargetWord( sentence_id, ap ) ] = false;
		}
		m_unaligned = true;
		for(char i=source_start; i<=source_end; i++) {
			if (!m_source_unaligned[ i ]) {
				m_unaligned = false;
			}
		}
	}
  ~Mismatch () {}

	bool Unaligned() { return m_unaligned; }
  void PrintClippedHTML( ostream* out, int width );
	void LabelSourceMatches( char *source_annotation, char *target_annotation, char source_id, char label );
};
