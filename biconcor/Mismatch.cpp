#include "Mismatch.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>

#include "SuffixArray.h"
#include "TargetCorpus.h"
#include "Alignment.h"
#include "Vocabulary.h"

using namespace std;

enum {
  UNANNOTATED = 0,
  PRE_ALIGNED = 1,
  POST_ALIGNED = 2,
  UNALIGNED = 3,
  MISALIGNED = 4,
  ALIGNED = 5
};

Mismatch::Mismatch( SuffixArray *sa, TargetCorpus *tc, Alignment *a, INDEX sentence_id, INDEX position, int source_length, int target_length, int source_start, int source_end )
  :m_suffixArray(sa)
  ,m_targetCorpus(tc)
  ,m_alignment(a)
  ,m_sentence_id(sentence_id)
  ,m_source_length(source_length)
  ,m_target_length(target_length)
  ,m_source_position(position)
  ,m_source_start(source_start)
  ,m_source_end(source_end)
  ,m_unaligned(true)
{
  // initialize unaligned indexes
  for (int i = 0; i < m_source_length; i++) {
    m_source_unaligned[i] = true;
  }
  for (int i = 0; i < m_target_length; i++) {
    m_target_unaligned[i] = true;
  }
  m_num_alignment_points =
    m_alignment->GetNumberOfAlignmentPoints( sentence_id );
  for(INDEX ap=0; ap<m_num_alignment_points; ap++) {
    m_source_unaligned[ (int)m_alignment->GetSourceWord( sentence_id, ap ) ] = false;
    m_target_unaligned[ (int)m_alignment->GetTargetWord( sentence_id, ap ) ] = false;
  }
  for(int i = source_start; i <= source_end; i++) {
    if (!m_source_unaligned[ i ]) {
      m_unaligned = false;
    }
  }
}

Mismatch::~Mismatch () {}

void Mismatch::PrintClippedHTML( ostream* out, int width )
{
  int source_annotation[256], target_annotation[256];
  vector< string > label_class;
  label_class.push_back( "" );
  label_class.push_back( "mismatch_pre_aligned" );
  label_class.push_back( "mismatch_post_aligned" );
  label_class.push_back( "null_aligned" );
  label_class.push_back( "mismatch_misaligned" );
  label_class.push_back( "mismatch_aligned" );

  for(int i=0; i<m_source_length; i++) source_annotation[i] = UNANNOTATED;
  for(int i=0; i<m_target_length; i++) target_annotation[i] = UNANNOTATED;

  if (m_unaligned) {
    // find alignment points for prior and next word(s) and
    // center target phrase around those.
    bool found_aligned = false;
    for(int i=1; i<m_source_length && !found_aligned; i++) {
      if (m_source_start-i >= 0) {
        int word_id =  m_source_start-i;
        source_annotation[ word_id ] = UNALIGNED;
        if (!m_source_unaligned[ word_id ]) {
          found_aligned = true;
          LabelSourceMatches( source_annotation, target_annotation, word_id, PRE_ALIGNED );
        }
      }

      if (m_source_end+i < m_source_length) {
        int word_id = m_source_end+i;
        source_annotation[ word_id ] = UNALIGNED;
        if (!m_source_unaligned[ word_id ]) {
          found_aligned = true;
          LabelSourceMatches( source_annotation, target_annotation, word_id, POST_ALIGNED );
        }
      }
    }

  }
  // misalignment
  else {
    // label aligned output words
    for(int i=m_source_start; i<=m_source_end; i++)
      LabelSourceMatches( source_annotation, target_annotation, i, ALIGNED );

    // find first and last
    int target_start = -1;
    int target_end = -1;
    for(int i=0; i<m_target_length; i++)
      if (target_annotation[i] == ALIGNED) {
        if (target_start == -1)
          target_start = i;
        target_end = i;
      }
    // go over all enclosed target words
    for(int i=target_start; i<=target_end; i++) {
      // label other target words as unaligned or misaligned
      if (m_target_unaligned[ i ])
        target_annotation[ i ] = UNALIGNED;
      else {
        if (target_annotation[ i ] != ALIGNED)
          target_annotation[ i ] = MISALIGNED;
        // loop over aligned source words
        for(INDEX ap=0; ap<m_num_alignment_points; ap++) {
          if (m_alignment->GetTargetWord( m_sentence_id, ap ) == i) {
            int source_word = m_alignment->GetSourceWord( m_sentence_id, ap );
            // if not part of the source phrase -> also misaligned
            if (source_word < m_source_start || source_word > m_source_end)
              source_annotation[ source_word ] = MISALIGNED;
          }
        }
      }
    }
    // closure
    bool change = true;
    while(change) {
      change = false;
      for(INDEX ap=0; ap<m_num_alignment_points; ap++) {
        int source_word = m_alignment->GetSourceWord( m_sentence_id, ap );
        int target_word = m_alignment->GetTargetWord( m_sentence_id, ap );
        if (source_annotation[source_word] != UNANNOTATED &&
            target_annotation[target_word] == UNANNOTATED) {
          target_annotation[target_word] = MISALIGNED;
          change = true;
        }
        if (source_annotation[source_word] == UNANNOTATED &&
            target_annotation[target_word] != UNANNOTATED) {
          source_annotation[source_word] = MISALIGNED;
          change = true;
        }
      }
    }
  }

  // print source
  // shorten source context if too long
  int sentence_start = m_source_position - m_source_start;
  int context_space = width/2;
  for(int i=m_source_start; i<=m_source_end; i++)
    context_space -= m_suffixArray->GetWord( sentence_start + i ).size() + 1;
  context_space /= 2;

  int remaining = context_space;
  int start_word = m_source_start;
  for(; start_word>0 && remaining>0; start_word--)
    remaining -= m_suffixArray->GetWord( sentence_start + start_word-1 ).size() + 1;
  if (remaining<0 || start_word == -1) start_word++;

  remaining = context_space;
  int end_word = m_source_end;
  for(; end_word<m_source_length && remaining>0; end_word++)
    remaining -= m_suffixArray->GetWord( sentence_start + end_word ).size() + 1;
  end_word--;

  // output with markup
  *out << "<tr><td class=\"pp_source_left\">";
  char current_label = UNANNOTATED;
  if (start_word>0) {
    current_label = source_annotation[start_word-1];
    *out << "... ";
  }
  for(int i=start_word; i<=end_word; i++) {
    // change to phrase block
    if (i == m_source_start) {
      if (current_label != UNANNOTATED && i!=start_word)
        *out << "</span>";
      *out << "</td><td class=\"pp_source\">";
      current_label = UNANNOTATED;
    }

    // change to labeled word
    else if (source_annotation[i] != current_label &&
             source_annotation[i] != ALIGNED) {
      if (current_label != UNANNOTATED && i!=start_word)
        *out << "</span>";
      if (source_annotation[i] != UNANNOTATED)
        *out << "<span class=\""
             << label_class[ source_annotation[i] ]
             << "\">";
      current_label = source_annotation[i];
    }

    // output word
    *out << m_suffixArray->GetWord( sentence_start + i ) << " ";

    // change to right context block
    if (i == m_source_end) {
      *out << "</td><td class=\"pp_source_right\">";
      current_label = UNANNOTATED;
    }
  }

  if (current_label != UNANNOTATED && end_word>m_source_end)
    *out << "</span>";
  if (end_word<m_source_length-1)
    *out << "... ";

  // print target
  // shorten target context if too long
  int target_start = -1;
  int target_end=0;
  for(int i=0; i<m_target_length; i++)
    if (target_annotation[i] != UNANNOTATED) {
      if (target_start == -1)
        target_start = i;
      target_end = i;
    }

  context_space = width/2;
  for(int i=target_start; i<=target_end; i++)
    context_space -= m_targetCorpus->GetWord( m_sentence_id, i ).size() + 1;
  while (context_space < 0) { // shorten matched part, if too long
    context_space +=
      m_targetCorpus->GetWord( m_sentence_id, target_start ).size() +
      m_targetCorpus->GetWord( m_sentence_id, target_end ).size() + 2;
    target_start++;
    target_end--;
  }
  context_space /= 2;

  remaining = context_space;
  start_word = target_start;
  for(; start_word>0 && remaining>0; start_word--) {
    //cerr << "remaining: " << remaining << ", start_word: " << start_word << endl;
    remaining -= m_targetCorpus->GetWord( m_sentence_id, start_word-1 ).size() + 1;
  }
  if (remaining<0 || start_word == -1) start_word++;

  remaining = context_space;
  end_word = target_end;
  for(; end_word<m_target_length && remaining>0; end_word++) {
    //cerr << "remaining: " << remaining << ", end_word: " << end_word << endl;
    remaining -= m_targetCorpus->GetWord( m_sentence_id, end_word ).size() + 1;
  }
  end_word--;

  // output with markup
  *out << "</td><td class=\"mismatch_target\">";
  current_label = UNANNOTATED;
  if (start_word>0) {
    current_label = target_annotation[start_word-1];
    *out << "... ";
  }
  for(int i=start_word; i<=end_word; i++) {
    if (target_annotation[i] != current_label) {
      if (current_label != UNANNOTATED && i!=start_word)
        *out << "</span>";
      if (target_annotation[i] != UNANNOTATED)
        *out << "<span class=\""
             << label_class[ target_annotation[i] ]
             << "\">";
      current_label = target_annotation[i];
    }

    // output word
    *out << m_targetCorpus->GetWord( m_sentence_id, i ) << " ";
  }

  if (current_label != UNANNOTATED && end_word>target_end)
    *out << "</span>";
  if (end_word<m_target_length-1)
    *out << "... ";
  *out << "</td></tr>";
}

void Mismatch::LabelSourceMatches(int *source_annotation, int *target_annotation, int source_id, int label )
{
  for(INDEX ap=0; ap<m_num_alignment_points; ap++) {
    if (m_alignment->GetSourceWord( m_sentence_id, ap ) == source_id) {
      source_annotation[ source_id ] = label;
      target_annotation[ m_alignment->GetTargetWord( m_sentence_id, ap ) ] = label;
    }
  }
}
