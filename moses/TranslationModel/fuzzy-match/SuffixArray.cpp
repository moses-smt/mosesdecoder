#include "SuffixArray.h"
#include <string>
#include <stdlib.h>
#include <cstring>

using namespace std;

namespace tmmt
{

SuffixArray::SuffixArray( string fileName )
{
  m_vcb.StoreIfNew( "<uNk>" );
  m_endOfSentence = m_vcb.StoreIfNew( "<s>" );

  ifstream extractFile;

  // count the number of words first;
  extractFile.open(fileName.c_str());
  istream *fileP = &extractFile;
  m_size = 0;
  size_t sentenceCount = 0;
  string line;
  while(getline(*fileP, line)) {

    vector< WORD_ID > words = m_vcb.Tokenize( line.c_str() );
    m_size += words.size() + 1;
    sentenceCount++;
  }
  extractFile.close();
  cerr << m_size << " words (incl. sentence boundaries)" << endl;

  // allocate memory
  m_array = (WORD_ID*) calloc( sizeof( WORD_ID ), m_size );
  m_index = (INDEX*) calloc( sizeof( INDEX ), m_size );
  m_wordInSentence = (char*) calloc( sizeof( char ), m_size );
  m_sentence = (size_t*) calloc( sizeof( size_t ), m_size );
  m_sentenceLength = (char*) calloc( sizeof( char ), sentenceCount );

  // fill the array
  int wordIndex = 0;
  int sentenceId = 0;
  extractFile.open(fileName.c_str());
  fileP = &extractFile;
  while(getline(*fileP, line)) {
    vector< WORD_ID > words = m_vcb.Tokenize( line.c_str() );

    // add to corpus vector
    corpus.push_back(words);

    // create SA

    vector< WORD_ID >::const_iterator i;
    for( i=words.begin(); i!=words.end(); i++) {
      m_index[ wordIndex ] = wordIndex;
      m_sentence[ wordIndex ] = sentenceId;
      m_wordInSentence[ wordIndex ] = i-words.begin();
      m_array[ wordIndex++ ] = *i;
    }
    m_index[ wordIndex ] = wordIndex;
    m_array[ wordIndex++ ] = m_endOfSentence;
    m_sentenceLength[ sentenceId++ ] = words.size();
  }
  extractFile.close();
  cerr << "done reading " << wordIndex << " words, " << sentenceId << " sentences." << endl;
  // List(0,9);

  // sort
  m_buffer = (INDEX*) calloc( sizeof( INDEX ), m_size );
  Sort( 0, m_size-1 );
  free( m_buffer );
  cerr << "done sorting" << endl;
}

// good ol' quick sort
void SuffixArray::Sort(INDEX start, INDEX end)
{
  if (start == end) return;
  INDEX mid = (start+end+1)/2;
  Sort( start, mid-1 );
  Sort( mid, end );

  // merge
  size_t i = start;
  size_t j = mid;
  size_t k = 0;
  size_t length = end-start+1;
  while( k<length ) {
    if (i == mid ) {
      m_buffer[ k++ ] = m_index[ j++ ];
    } else if (j > end ) {
      m_buffer[ k++ ] = m_index[ i++ ];
    } else {
      if (CompareIndex( m_index[i], m_index[j] ) < 0) {
        m_buffer[ k++ ] = m_index[ i++ ];
      } else {
        m_buffer[ k++ ] = m_index[ j++ ];
      }
    }
  }

  memcpy( ((char*)m_index) + sizeof( INDEX ) * start,
          ((char*)m_buffer), sizeof( INDEX ) * (end-start+1) );
}

SuffixArray::~SuffixArray()
{
  free(m_index);
  free(m_array);
}

int SuffixArray::CompareIndex( INDEX a, INDEX b ) const
{
  // skip over identical words
  INDEX offset = 0;
  while( a+offset < m_size &&
         b+offset < m_size &&
         m_array[ a+offset ] == m_array[ b+offset ] ) {
    offset++;
  }

  if( a+offset == m_size ) return -1;
  if( b+offset == m_size ) return 1;
  return CompareWord( m_array[ a+offset ], m_array[ b+offset ] );
}

inline int SuffixArray::CompareWord( WORD_ID a, WORD_ID b ) const
{
  // cerr << "c(" << m_vcb.GetWord(a) << ":" << m_vcb.GetWord(b) << ")=" << m_vcb.GetWord(a).compare( m_vcb.GetWord(b) ) << endl;
  return m_vcb.GetWord(a).compare( m_vcb.GetWord(b) );
}

int SuffixArray::Count( const vector< WORD > &phrase )
{
  INDEX dummy;
  return LimitedCount( phrase, m_size, dummy, dummy, 0, m_size-1 );
}

bool SuffixArray::MinCount( const vector< WORD > &phrase, INDEX min )
{
  INDEX dummy;
  return LimitedCount( phrase, min, dummy, dummy, 0, m_size-1 ) >= min;
}

bool SuffixArray::Exists( const vector< WORD > &phrase )
{
  INDEX dummy;
  return LimitedCount( phrase, 1, dummy, dummy, 0, m_size-1 ) == 1;
}

int SuffixArray::FindMatches( const vector< WORD > &phrase, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start, INDEX search_end )
{
  return LimitedCount( phrase, m_size, firstMatch, lastMatch, search_start, search_end );
}

int SuffixArray::LimitedCount( const vector< WORD > &phrase, INDEX min, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start, INDEX search_end )
{
  // cerr << "FindFirst\n";
  INDEX start = search_start;
  INDEX end = (search_end == -1) ? (m_size-1) : search_end;
  INDEX mid = FindFirst( phrase, start, end );
  // cerr << "done\n";
  if (mid == m_size) return 0; // no matches
  if (min == 1) return 1;      // only existance check

  int matchCount = 1;

  //cerr << "before...\n";
  firstMatch = FindLast( phrase, mid, start, -1 );
  matchCount += mid - firstMatch;

  //cerr << "after...\n";
  lastMatch = FindLast( phrase, mid, end, 1 );
  matchCount += lastMatch - mid;

  return matchCount;
}

SuffixArray::INDEX SuffixArray::FindLast( const vector< WORD > &phrase, INDEX start, INDEX end, int direction )
{
  end += direction;
  while(true) {
    INDEX mid = ( start + end + (direction>0 ? 0 : 1) )/2;

    int match = Match( phrase, mid );
    int matchNext = Match( phrase, mid+direction );
    //cerr << "\t" << start << ";" << mid << ";" << end << " -> " << match << "," << matchNext << endl;

    if (match == 0 && matchNext != 0) return mid;

    if (match == 0) // mid point is a match
      start = mid;
    else
      end = mid;
  }
}

SuffixArray::INDEX SuffixArray::FindFirst( const vector< WORD > &phrase, INDEX &start, INDEX &end )
{
  while(true) {
    INDEX mid = ( start + end + 1 )/2;
    //cerr << "FindFirst(" << start << ";" << mid << ";" << end << ")\n";
    int match = Match( phrase, mid );

    if (match == 0) return mid;
    if (start >= end && match != 0 ) return m_size;

    if (match > 0)
      start = mid+1;
    else
      end = mid-1;
  }
}

int SuffixArray::Match( const vector< WORD > &phrase, INDEX index )
{
  INDEX pos = m_index[ index ];
  for(INDEX i=0; i<phrase.size() && i+pos<m_size; i++) {
    int match = CompareWord( m_vcb.GetWordID( phrase[i] ), m_array[ pos+i ] );
    // cerr << "{" << index << "+" << i << "," << pos+i << ":" << match << "}" << endl;
    if (match != 0)
      return match;
  }
  return 0;
}

void SuffixArray::List(INDEX start, INDEX end)
{
  for(INDEX i=start; i<=end; i++) {
    INDEX pos = m_index[ i ];
    // cerr << i << ":" << pos << "\t";
    for(int j=0; j<5 && j+pos<m_size; j++) {
      //cout << " " << m_vcb.GetWord( m_array[ pos+j ] );
    }
    // cerr << "\n";
  }
}

}

