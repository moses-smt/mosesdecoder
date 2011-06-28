#include "PhrasePairCollection.h"
#include <string>
#include <stdlib.h>
#include <cstring>
#include <algorithm>

using namespace std;

PhrasePairCollection::PhrasePairCollection( SuffixArray *sa, TargetCorpus *tc, Alignment *a )
:m_suffixArray(sa)
,m_targetCorpus(tc)
,m_alignment(a)
,m_size(0)
,m_max_lookup(10000)
,m_max_pp_target(50)
,m_max_pp(50)
{}

PhrasePairCollection::~PhrasePairCollection()
{}

bool PhrasePairCollection::GetCollection( const vector< string > sourceString ) {
	INDEX first_match, last_match;
	if (! m_suffixArray->FindMatches( sourceString, first_match, last_match )) {
		return false;
	}
	cerr << "\tfirst match " << first_match << endl;
	cerr << "\tlast match " << last_match << endl;
	
	INDEX found = last_match - first_match +1;

	map< vector< WORD_ID >, INDEX > index;	
	for( INDEX i=first_match; i<=last_match; i++ ) {
		int position = m_suffixArray->GetPosition( i );
		int source_start = m_suffixArray->GetWordInSentence( position );
		int source_end = source_start + sourceString.size()-1;
		INDEX sentence_id = m_suffixArray->GetSentence( position );
		int sentence_length = m_suffixArray->GetSentenceLength( sentence_id );
		int target_length = m_targetCorpus->GetSentenceLength( sentence_id );
		cerr << "match " << (i-first_match) 
		     << " in sentence " << sentence_id 
		     << ", starting at word " << source_start
		     << " of " << sentence_length
		     << ". target sentence has " << target_length << " words.";
		char target_start, target_end, pre_null, post_null;
		if (m_alignment->PhraseAlignment( sentence_id, target_length, source_start, source_end, target_start, target_end, pre_null, post_null)) {
			cerr << " aligned to [" << (int)target_start << "," << (int)target_end << "]";
			cerr << " +(" << (int)pre_null << "," << (int)post_null << ")";
			for( char pre = 0; pre <= pre_null; pre++ ) {
				for( char post = 0; post <= post_null; post++ ) {
					vector< WORD_ID > targetString;
					cerr << "; ";
					for( char target = target_start-pre; target <= target_end+post; target++ ) {	
						targetString.push_back( m_targetCorpus->GetWordId( sentence_id, target) );
						cerr << m_targetCorpus->GetWord( sentence_id, target) << " ";
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
		}
		cerr << endl;

		if (found > m_max_lookup) {
			i += found/m_max_lookup-1;
		}
	}
	sort(m_collection.begin(), m_collection.end(), CompareBySize());
}

void PhrasePairCollection::Print() {
	vector< vector<PhrasePair*> >::iterator ppWithSameTarget;
	for( ppWithSameTarget = m_collection.begin(); ppWithSameTarget != m_collection.end(); ppWithSameTarget++ ) {
		(*(ppWithSameTarget->begin()))->PrintTarget( &cout );
		int count = ppWithSameTarget->size();
		cout << "(" << count << ")" << endl;
		vector< PhrasePair* >::iterator p;
		for(p = ppWithSameTarget->begin(); p != ppWithSameTarget->end(); p++ ) {
			(*p)->Print( &cout, 100 );
		}
	}
}

void PhrasePairCollection::PrintHTML() {
	vector< vector<PhrasePair*> >::iterator ppWithSameTarget;
	int pp_target = 0;
	for( ppWithSameTarget = m_collection.begin(); ppWithSameTarget != m_collection.end() && pp_target<m_max_pp_target; ppWithSameTarget++, pp_target++ ) {
		cout << "<p class=\"pp_target_header\">";
		(*(ppWithSameTarget->begin()))->PrintTarget( &cout );
		int count = ppWithSameTarget->size();
		cout << "(" << count << "/" << m_size << ")" << endl;
		cout << "<p><table align=\"center\">";
		vector< PhrasePair* >::iterator p;
		int pp = 0;
		for(p = ppWithSameTarget->begin(); pp<count && p != ppWithSameTarget->end(); p++, pp++ ) {
			(*p)->PrintClippedHTML( &cout, 160 );
			if (count > m_max_pp) {
				p += count/m_max_pp-1;
				pp += count/m_max_pp-1;
			} 
		}
		cout << "</table>\n";
	}
}
