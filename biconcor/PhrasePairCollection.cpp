#include "PhrasePairCollection.h"

#include <stdlib.h>
#include <cstring>
#include <algorithm>

#include "Vocabulary.h"
#include "SuffixArray.h"
#include "TargetCorpus.h"
#include "Alignment.h"
#include "PhrasePair.h"
#include "Mismatch.h"

using namespace std;

PhrasePairCollection::PhrasePairCollection( SuffixArray *sa, TargetCorpus *tc, Alignment *a, int max_translation, int max_example )
  :m_suffixArray(sa)
  ,m_targetCorpus(tc)
  ,m_alignment(a)
  ,m_size(0)
  ,m_max_lookup(10000)          // maximum number of source occurrences sampled
  ,m_max_translation(max_translation)    // max number of different distinct translations returned
  ,m_max_example(max_example) // max number of examples returned for each distinct translation
{}

PhrasePairCollection::~PhrasePairCollection()
{}

int PhrasePairCollection::GetCollection( const vector< string >& sourceString )
{
  INDEX first_match, last_match;
  if (! m_suffixArray->FindMatches( sourceString, first_match, last_match )) {
    return 0;
  }
  //cerr << "\tfirst match " << first_match << endl;
  //cerr << "\tlast match " << last_match << endl;

  INDEX found = last_match - first_match +1;

  map< vector< WORD_ID >, INDEX > index;
  int real_count = 0;
  for( INDEX i=first_match; i<=last_match; i++ ) {
    int position = m_suffixArray->GetPosition( i );
    int source_start = m_suffixArray->GetWordInSentence( position );
    int source_end = source_start + sourceString.size()-1;
    INDEX sentence_id = m_suffixArray->GetSentence( position );
    int sentence_length = m_suffixArray->GetSentenceLength( sentence_id );
    int target_length = m_targetCorpus->GetSentenceLength( sentence_id );
    //cerr << "match " << (i-first_match)
    //<< " in sentence " << sentence_id
    //<< ", starting at word " << source_start
    //<< " of " << sentence_length
    //<< ". target sentence has " << target_length << " words.";
    int target_start, target_end, pre_null, post_null;
    if (m_alignment->PhraseAlignment( sentence_id, target_length, source_start, source_end, target_start, target_end, pre_null, post_null)) {
      //cerr << " aligned to [" << (int)target_start << "," << (int)target_end << "]";
      //cerr << " +(" << (int)pre_null << "," << (int)post_null << ")";
      bool null_boundary_words = false;
      for (int pre = 0; pre <= pre_null && (pre == 0 || null_boundary_words); pre++ ) {
        for (int post = 0; post <= post_null && (post == 0 || null_boundary_words); post++ ) {
          vector< WORD_ID > targetString;
          //cerr << "; ";
          for (int target = target_start - pre; target <= target_end + post; target++) {
            targetString.push_back( m_targetCorpus->GetWordId( sentence_id, target) );
            //cerr << m_targetCorpus->GetWord( sentence_id, target) << " ";
          }
          PhrasePair *phrasePair = new PhrasePair( m_suffixArray, m_targetCorpus, m_alignment, sentence_id, target_length, position, source_start, source_end, target_start-pre, target_end+post, pre, post, pre_null-pre, post_null-post);
          // matchCollection.Add( sentence_id, )
          if (index.find( targetString ) == index.end()) {
            index[targetString] = m_collection.size();
            vector< PhrasePair* > emptyVector;
            m_collection.push_back( emptyVector );
          }
          m_collection[ index[targetString] ].push_back( phrasePair );
          m_size++;
        }
      }
    } else {
      //cerr << "mismatch " << (i-first_match)
      //		 << " in sentence " << sentence_id
      //		 << ", starting at word " << source_start
      //		 << " of " << sentence_length
      //		 << ". target sentence has " << target_length << " words.";
      Mismatch *mismatch = new Mismatch( m_suffixArray, m_targetCorpus, m_alignment, sentence_id, position, sentence_length, target_length, source_start, source_end );
      if (mismatch->Unaligned())
        m_unaligned.push_back( mismatch );
      else
        m_mismatch.push_back( mismatch );
    }
    //cerr << endl;

    if (found > (INDEX)m_max_lookup) {
      i += found/m_max_lookup-1;
    }
    real_count++;
  }
  sort(m_collection.begin(), m_collection.end(), CompareBySize());
  return real_count;
}

void PhrasePairCollection::Print(bool pretty) const
{
  vector< vector<PhrasePair*> >::const_iterator ppWithSameTarget;
  int i=0;
  for( ppWithSameTarget = m_collection.begin(); ppWithSameTarget != m_collection.end() && i<m_max_translation; i++, ppWithSameTarget++ ) {
    (*(ppWithSameTarget->begin()))->PrintTarget( &cout );
    int count = ppWithSameTarget->size();
    cout << "(" << count << ")" << endl;
    vector< PhrasePair* >::const_iterator p = ppWithSameTarget->begin();
    for(int j=0; j<ppWithSameTarget->size() && j<m_max_example; j++, p++ ) {
      if (pretty) {
        (*p)->PrintPretty( &cout, 100 );
      } else {
        (*p)->Print( &cout );
      }
      if (ppWithSameTarget->size() > m_max_example) {
        p += ppWithSameTarget->size()/m_max_example-1;
      }
    }
  }
}

void PhrasePairCollection::PrintHTML() const
{
  int pp_target = 0;
  bool singleton = false;
  // loop over all translations
  vector< vector<PhrasePair*> >::const_iterator ppWithSameTarget;
  for( ppWithSameTarget = m_collection.begin(); ppWithSameTarget != m_collection.end() && pp_target<m_max_translation; ppWithSameTarget++, pp_target++ ) {

    int count = ppWithSameTarget->size();
    if (!singleton) {
      if (count == 1) {
        singleton = true;
        cout << "<p class=\"pp_singleton_header\">singleton"
             << (m_collection.end() - ppWithSameTarget==1?"":"s") << " ("
             << (m_collection.end() - ppWithSameTarget)
             << "/" << m_size << ")</p>";
      } else {
        cout << "<p class=\"pp_target_header\">";
        (*(ppWithSameTarget->begin()))->PrintTarget( &cout );
        cout << " (" << count << "/" << m_size << ")" << endl;
        cout << "<p><div id=\"pp_" << pp_target << "\">";
      }
      cout << "<table align=\"center\">";
    }

    vector< PhrasePair* >::const_iterator p;
    // loop over all sentences where translation occurs
    int pp=0;
    int i=0;
    for(p = ppWithSameTarget->begin(); i<10 && pp<count && p != ppWithSameTarget->end(); p++, pp++, i++ ) {
      (*p)->PrintClippedHTML( &cout, 160 );
      if (count > m_max_example) {
        p += count/m_max_example-1;
        pp += count/m_max_example-1;
      }
    }
    if (i == 10 && pp < count) {
      // extended table
      cout << "<tr><td colspan=7 align=center class=\"pp_more\" onclick=\"javascript:document.getElementById('pp_" << pp_target << "').style.display = 'none'; document.getElementById('pp_ext_" << pp_target << "').style.display = 'block';\">(more)</td></tr></table></div>";
      cout << "<div id=\"pp_ext_" << pp_target << "\" style=\"display:none;\";\">";
      cout << "<table align=\"center\">";
      for(i=0, pp=0, p = ppWithSameTarget->begin(); i<m_max_example && pp<count && p != ppWithSameTarget->end(); p++, pp++, i++ ) {
        (*p)->PrintClippedHTML( &cout, 160 );
        if (count > m_max_example) {
          p += count/m_max_example-1;
          pp += count/m_max_example-1;
        }
      }
    }
    if (!singleton) cout << "</table></div>\n";

    if (!singleton && pp_target == 9) {
      cout << "<div id=\"pp_toggle\" onclick=\"javascript:document.getElementById('pp_toggle').style.display = 'none'; document.getElementById('pp_additional').style.display = 'block';\">";
      cout << "<p class=\"pp_target_header\">(more)</p></div>";
      cout << "<div id=\"pp_additional\" style=\"display:none;\";\">";
    }
  }
  if (singleton) cout << "</table></div>\n";
  else if (pp_target > 9)	cout << "</div>";

  size_t max_mismatch = m_max_example/3;
  // unaligned phrases
  if (m_unaligned.size() > 0) {
    cout << "<p class=\"pp_singleton_header\">unaligned"
         << " (" << (m_unaligned.size()) << ")</p>";
    cout << "<table align=\"center\">";
    int step_size = 1;
    if (m_unaligned.size() > max_mismatch)
      step_size = (m_unaligned.size()+max_mismatch-1) / max_mismatch;
    for(size_t i=0; i<m_unaligned.size(); i+=step_size)
      m_unaligned[i]->PrintClippedHTML( &cout, 160 );
    cout << "</table>";
  }

  // mismatched phrases
  if (m_mismatch.size() > 0) {
    cout << "<p class=\"pp_singleton_header\">mismatched"
         << " (" << (m_mismatch.size()) << ")</p>";
    cout << "<table align=\"center\">";
    int step_size = 1;
    if (m_mismatch.size() > max_mismatch)
      step_size = (m_mismatch.size()+max_mismatch-1) / max_mismatch;
    for(size_t i=0; i<m_mismatch.size(); i+=step_size)
      m_mismatch[i]->PrintClippedHTML( &cout, 160 );
    cout << "</table>";
  }
}
