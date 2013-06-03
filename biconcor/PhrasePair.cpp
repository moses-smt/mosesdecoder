#include "PhrasePair.h"

#include <iostream>
#include "TargetCorpus.h"
#include "Alignment.h"
#include "Vocabulary.h"
#include "SuffixArray.h"

using namespace std;

void PhrasePair::Print( ostream* out ) const
{
  // source
  int sentence_start = m_source_position - m_source_start;
  char source_length = m_suffixArray->GetSentenceLength( m_suffixArray->GetSentence( m_source_position ) );

  for( char i=0; i<source_length; i++ ) {
    if (i>0) *out << " ";
    *out << m_suffixArray->GetWord( sentence_start + i );
  }

  // target
  *out << " |||";
  for( char i=0; i<m_target_length; i++ ) {
    *out << " " << m_targetCorpus->GetWord( m_sentence_id, i);
  }

  // source span
  *out << " ||| " << (int)m_source_start << " " << (int)m_source_end;

  // target span
  *out << " ||| " << (int)m_target_start << " " << (int)m_target_end;

  // word alignment
  *out << " |||";

  INDEX ap_points = m_alignment->GetNumberOfAlignmentPoints( m_sentence_id );
  for( INDEX i=0; i<ap_points; i++) {
    *out << " " << m_alignment->GetSourceWord( m_sentence_id, i )
         << "-" << m_alignment->GetTargetWord( m_sentence_id, i );
  }

  *out << endl;
}

void PhrasePair::PrintPretty( ostream* out, int width ) const
{
  vector< WORD_ID >::const_iterator t;

  // source
  int sentence_start = m_source_position - m_source_start;
  size_t source_width = (width-3)/2;
  string source_pre = "";
  string source = "";
  string source_post = "";
  for( size_t space=0; space<source_width/2; space++ ) source_pre += " ";
  for( char i=0; i<m_source_start; i++ ) {
    source_pre += " " + m_suffixArray->GetWord( sentence_start + i );
  }
  for( char i=m_source_start; i<=m_source_end; i++ ) {
    if (i>m_source_start) source += " ";
    source += m_suffixArray->GetWord( sentence_start + i );
  }
  char source_length = m_suffixArray->GetSentenceLength( m_suffixArray->GetSentence( m_source_position ) );
  for( char i=m_source_end+1; i<source_length; i++ ) {
    if (i>m_source_end+1) source_post += " ";
    source_post += m_suffixArray->GetWord( sentence_start + i );
  }
  for( size_t space=0; space<source_width/2; space++ ) source_post += " ";

  size_t source_pre_width = (source_width-source.size()-2)/2;
  size_t source_post_width = (source_width-source.size()-2+1)/2;

  if (source.size() > (size_t)width) {
    source_pre_width = 0;
    source_post_width = 0;
  }

  *out << source_pre.substr( source_pre.size()-source_pre_width, source_pre_width ) << " "
       << source.substr( 0, source_width -2 ) << " "
       << source_post.substr( 0, source_post_width ) << " | ";

  // target
  size_t target_width = (width-3)/2;
  string target_pre = "";
  string target = "";
  string target_post = "";
  for( size_t space=0; space<target_width/2; space++ ) target_pre += " ";
  for( char i=0; i<m_target_start; i++ ) {
    target_pre += " " + m_targetCorpus->GetWord( m_sentence_id, i);
  }
  for( char i=m_target_start; i<=m_target_end; i++ ) {
    if (i>m_target_start) target += " ";
    target += m_targetCorpus->GetWord( m_sentence_id, i);
  }
  for( char i=m_target_end+1; i<m_target_length; i++ ) {
    if (i>m_target_end+1) target_post += " ";
    target_post += m_targetCorpus->GetWord( m_sentence_id, i);
  }

  size_t target_pre_width = (target_width-target.size()-2)/2;
  size_t target_post_width = (target_width-target.size()-2+1)/2;

  if (target.size() > (size_t)width) {
    target_pre_width = 0;
    target_post_width = 0;
  }

  *out << target_pre.substr( target_pre.size()-target_pre_width, target_pre_width ) << " "
       << target.substr( 0, target_width -2 ) << " "
       << target_post.substr( 0, target_post_width ) << endl;
}

void PhrasePair::PrintTarget( ostream* out ) const
{
  for( char i=m_target_start; i<=m_target_end; i++ ) {
    if (i>m_target_start) *out << " ";
    *out << m_targetCorpus->GetWord( m_sentence_id, i);
  }
}

void PhrasePair::PrintHTML( ostream* out ) const
{
  // source
  int sentence_start = m_source_position - m_source_start;
  char source_length = m_suffixArray->GetSentenceLength( m_suffixArray->GetSentence( m_source_position ) );

  *out << "<tr><td align=right class=\"pp_source_left\">";
  for( char i=0; i<m_source_start; i++ ) {
    if (i>0) *out << " ";
    *out << m_suffixArray->GetWord( sentence_start + i );
  }
  *out << "</td><td class=\"pp_source\">";
  for( char i=m_source_start; i<=m_source_end; i++ ) {
    if (i>m_source_start) *out << " ";
    *out << m_suffixArray->GetWord( sentence_start + i );
  }
  *out << "</td><td class=\"pp_source_right\">";
  for( char i=m_source_end+1; i<source_length; i++ ) {
    if (i>m_source_end+1) *out << " ";
    *out << m_suffixArray->GetWord( sentence_start + i );
  }

  // target
  *out << "</td><td class=\"pp_target_left\">";
  for( char i=0; i<m_target_start; i++ ) {
    if (i>0) *out << " ";
    *out << m_targetCorpus->GetWord( m_sentence_id, i);
  }
  *out << "</td><td class=\"pp_target\">";
  for( char i=m_target_start; i<=m_target_end; i++ ) {
    if (i>m_target_start) *out << " ";
    *out << m_targetCorpus->GetWord( m_sentence_id, i);
  }
  *out << "</td><td class=\"pp_target_right\">";
  for( char i=m_target_end+1; i<m_target_length; i++ ) {
    if (i>m_target_end+1) *out << " ";
    *out << m_targetCorpus->GetWord( m_sentence_id, i);
  }
  *out << "</td></tr>\n";
}

void PhrasePair::PrintClippedHTML( ostream* out, int width ) const
{
  vector< WORD_ID >::const_iterator t;

  // source
  int sentence_start = m_source_position - m_source_start;
  size_t source_width = (width+1)/2;
  string source_pre = "";
  string source = "";
  string source_post = "";
  for( char i=0; i<m_source_start; i++ ) {
    source_pre += " " + m_suffixArray->GetWord( sentence_start + i );
  }
  for( char i=m_source_start; i<=m_source_end; i++ ) {
    if (i>m_source_start) source += " ";
    source += m_suffixArray->GetWord( sentence_start + i );
  }
  char source_length = m_suffixArray->GetSentenceLength( m_suffixArray->GetSentence( m_source_position ) );
  for( char i=m_source_end+1; i<source_length; i++ ) {
    if (i>m_source_end+1) source_post += " ";
    source_post += m_suffixArray->GetWord( sentence_start + i );
  }
  size_t source_pre_width = (source_width-source.size())/2;
  size_t source_post_width = (source_width-source.size()+1)/2;

  // if phrase is too long, don't show any context
  if (source.size() > (size_t)width) {
    source_pre_width = 0;
    source_post_width = 0;
  }
  // too long -> truncate and add "..."
  if (source_pre.size() > source_pre_width) {
    // first skip up to a space
    while(source_pre_width>0 &&
          source_pre.substr(source_pre.size()-source_pre_width,1) != " ") {
      source_pre_width--;
    }
    source_pre = "..." + source_pre.substr( source_pre.size()-source_pre_width, source_pre_width );
  }
  if (source_post.size() > source_post_width) {
    while(source_post_width>0 &&
          source_post.substr(source_post_width-1,1) != " ") {
      source_post_width--;
    }
    source_post = source_post.substr( 0, source_post_width ) + "...";
  }

  *out << "<tr><td class=\"pp_source_left\">"
       << source_pre
       << "</td><td class=\"pp_source\">"
       << source.substr( 0, source_width -2 )
       << "</td><td class=\"pp_source_right\">"
       << source_post
       << "</td>";

  // target
  size_t target_width = width/2;
  string target_pre = "";
  string target = "";
  string target_post = "";
  size_t target_pre_null_width = 0;
  size_t target_post_null_width = 0;
  for( char i=0; i<m_target_start; i++ ) {
    WORD word = m_targetCorpus->GetWord( m_sentence_id, i);
    target_pre += " " + word;
    if (i >= m_target_start-m_pre_null)
      target_pre_null_width += word.size() + 1;
  }
  for( char i=m_target_start; i<=m_target_end; i++ ) {
    if (i>m_target_start) target += " ";
    target += m_targetCorpus->GetWord( m_sentence_id, i);
  }
  for( char i=m_target_end+1; i<m_target_length; i++ ) {
    if (i>m_target_end+1) target_post += " ";
    WORD word = m_targetCorpus->GetWord( m_sentence_id, i);
    target_post += word;
    if (i-(m_target_end+1) < m_post_null) {
      target_post_null_width += word.size() + 1;
    }
  }

  size_t target_pre_width = (target_width-target.size())/2;
  size_t target_post_width = (target_width-target.size()+1)/2;

  if (target.size() > (size_t)width) {
    target_pre_width = 0;
    target_post_width = 0;
  }

  if (target_pre.size() < target_pre_width)
    target_pre_width = target_pre.size();
  else {
    while(target_pre_width>0 &&
          target_pre.substr(target_pre.size()-target_pre_width,1) != " ") {
      target_pre_width--;
    }
    target_pre = "..." + target_pre.substr( target_pre.size()-target_pre_width, target_pre_width );
  }

  if (target_post.size() < target_post_width) {
    target_post_width = target_post.size();
  } else {
    while(target_post_width>0 &&
          target_post.substr(target_post_width-1,1) != " ") {
      target_post_width--;
    }
    target_post = target_post.substr( 0, target_post_width ) + "...";
  }

  if (m_pre_null) {
    //cerr << endl << "target_pre_width=" << target_pre_width << ", target_pre_null_width=" << target_pre_null_width << ", target_pre.size()=" << target_pre.size() << endl;
    if (target_pre_width < target_pre.size())
      target_pre_null_width -= target_pre.size()-target_pre_width;
    target_pre = target_pre.substr(0,target_pre_width-target_pre_null_width)
                 + "<span class=\"null_aligned\">"
                 + target_pre.substr(target_pre_width-target_pre_null_width)
                 + "</span>";
  }
  if (m_post_null) {
    //cerr << endl << "target_post_width=" << target_post_width << ", target_post_null_width=" << target_post_null_width << ", target_post.size()=" << target_post.size() << endl;
    if (target_post_null_width > target_post.size()) {
      target_post_null_width = target_post.size();
    }
    target_post = "<span class=\"null_aligned\">"
                  + target_post.substr(0,target_post_null_width)
                  + "</span>"
                  + target_post.substr(target_post_null_width);
  }

  *out << "<td class=\"pp_target_left\">"
       << target_pre
       << "</td><td class=\"pp_target\">"
       << target.substr( 0, target_width -2 )
       << "</td><td class=\"pp_target_right\">"
       << target_post
       << "</td></tr>"<< endl;
}

