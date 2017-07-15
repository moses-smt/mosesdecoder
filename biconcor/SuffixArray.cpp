#include "SuffixArray.h"

#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>

namespace
{

const int LINE_MAX_LENGTH = 10000;

} // namespace

using namespace std;

SuffixArray::SuffixArray()
  : m_array(NULL),
    m_index(NULL),
    m_buffer(NULL),
    m_wordInSentence(NULL),
    m_sentence(NULL),
    m_sentenceLength(NULL),
    m_document(NULL),
    m_documentName(NULL),
    m_documentNameLength(0),
    m_documentCount(0),
    m_useDocument(false),
    m_vcb(),
    m_size(0),
    m_sentenceCount(0) { }

SuffixArray::~SuffixArray()
{
  free(m_array);
  free(m_index);
  free(m_wordInSentence);
  free(m_sentence);
  free(m_sentenceLength);
  free(m_document);
  free(m_documentName);
}

void SuffixArray::Create(const string& fileName )
{
  m_vcb.StoreIfNew( "<uNk>" );
  m_endOfSentence = m_vcb.StoreIfNew( "<s>" );

  ifstream textFile;
  char line[LINE_MAX_LENGTH];

  // count the number of words first;
  textFile.open(fileName.c_str());

  if (!textFile) {
    cerr << "Error: no such file or directory " << fileName << endl;
    exit(1);
  }

  // first pass through data: get size
  istream *fileP = &textFile;
  m_size = 0;
  m_sentenceCount = 0;
  m_documentCount = 0;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, LINE_MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    if (m_useDocument && ProcessDocumentLine(line,0)) continue;
    vector< WORD_ID > words = m_vcb.Tokenize( line );
    m_size += words.size() + 1;
    m_sentenceCount++;
  }
  textFile.close();
  cerr << m_size << " words (incl. sentence boundaries)" << endl;
  if (m_useDocument) {
    cerr << m_documentCount << " documents" << endl;
    if (m_documentCount == 0) {
      cerr << "Error: no documents found, aborting." << endl;
      exit(1);
    }
  }

  // allocate memory
  m_array = (WORD_ID*) calloc( sizeof( WORD_ID ), m_size );
  m_index = (INDEX*) calloc( sizeof( INDEX ), m_size );
  m_wordInSentence = (char*) calloc( sizeof( char ), m_size );
  m_sentence = (INDEX*) calloc( sizeof( INDEX ), m_size );
  m_sentenceLength = (char*) calloc( sizeof( char ), m_sentenceCount );
  CheckAllocation(m_array != NULL, "m_array");
  CheckAllocation(m_index != NULL, "m_index");
  CheckAllocation(m_wordInSentence != NULL, "m_wordInSentence");
  CheckAllocation(m_sentence != NULL, "m_sentence");
  CheckAllocation(m_sentenceLength != NULL, "m_sentenceLength");
  if (m_useDocument) {
    m_document = (INDEX*) calloc( sizeof( INDEX ), m_documentCount );
    m_documentName = (INDEX*) calloc( sizeof( INDEX ), m_documentCount );
    m_documentNameBuffer = (char*) calloc( sizeof( char ), m_documentNameLength );
    CheckAllocation(m_document != NULL, "m_document");
    CheckAllocation(m_documentName != NULL, "m_documentName");
    CheckAllocation(m_documentNameBuffer != NULL, "m_documentNameBuffer");
  }

  // second pass through data: fill the arrays
  int wordIndex = 0;
  int sentenceId = 0;
  m_documentNameLength = 0; // re-use as counter
  m_documentCount = 0;      // re-use as counter
  textFile.open(fileName.c_str());
  fileP = &textFile;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, LINE_MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    if (m_useDocument && ProcessDocumentLine(line,sentenceId)) continue;
    vector< WORD_ID > words = m_vcb.Tokenize( line );
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
  textFile.close();
  cerr << "done reading " << wordIndex << " words, " << sentenceId << " sentences." << endl;
  // List(0,9);

  // sort
  m_buffer = (INDEX*) calloc( sizeof( INDEX ), m_size );

  if (m_buffer == NULL) {
    cerr << "Error: cannot allocate memory to m_buffer" << endl;
    exit(1);
  }

  Sort( 0, m_size-1 );
  free( m_buffer );
  cerr << "done sorting" << endl;
}

// very specific code to deal with common crawl document ids
bool SuffixArray::ProcessDocumentLine( const char *line, const size_t sentenceId )
{
  size_t i;
  // first 32 characters are hex-hash
  for(i=0; i<32; i++) {
    if ((line[i] < '0' || line[i] > '9') && (line[i] < 'a' || line[i] > 'f')) {
      return false;
    }
  }
  if (line[i++] != ' ') return false;

  // second token is float
  for (; line[i] != ' ' && line[i] != 0; i++) {
    if (line[i] != '.' && (line[i] < '0' || line[i] > '9')) {
      return false;
    }
  }
  i++;

  // last token is url (=name)
  size_t startName = i;
  for (; line[i] != ' ' && line[i] != 0; i++) {}
  if (line[i] == ' ') return false;
  size_t endName = i+1; // include '\0'

  // second pass: record name and sentence number
  if (m_document != NULL) {
    m_documentName[m_documentCount] = m_documentNameLength;
    for(size_t i=startName; i<endName; i++) {
      m_documentNameBuffer[m_documentNameLength + i-startName] = line[i];
    }
    m_document[m_documentCount] = sentenceId;
  }
  m_documentNameLength += endName-startName;
  m_documentCount++;
  return true;
}

// good ol' quick sort
void SuffixArray::Sort(INDEX start, INDEX end)
{
  if (start == end) return;
  INDEX mid = (start+end+1)/2;
  Sort( start, mid-1 );
  Sort( mid, end );

  // merge
  INDEX i = start;
  INDEX j = mid;
  INDEX k = 0;
  INDEX length = end-start+1;
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
  return (INDEX)LimitedCount( phrase, min, dummy, dummy, 0, m_size-1 ) >= min;
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
  INDEX end = (search_end == (INDEX)-1) ? (m_size-1) : search_end;
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
      cout << " " << m_vcb.GetWord( m_array[ pos+j ] );
    }
    // cerr << "\n";
  }
}

void SuffixArray::PrintSentenceMatches( const std::vector< WORD > &phrase )
{
  cout << "QUERY\t";
  for(size_t i=0; i<phrase.size(); i++) {
    if (i>0) cout << " ";
    cout <<  phrase[i];
  }
  cout << '\t';
  INDEX start = 0;
  INDEX end = m_size-1;
  INDEX mid = FindFirst( phrase, start, end );
  if (mid == m_size) { // no matches
    cout << "0 matches" << endl;
    return;
  }

  INDEX firstMatch = FindLast( phrase, mid, start, -1 );
  INDEX lastMatch = FindLast( phrase, mid, end, 1 );

  // loop through all matches
  cout << (lastMatch-firstMatch+1) << " matches" << endl;
  for(INDEX i=firstMatch; i<=lastMatch; i++) {
    // get sentence information
    INDEX pos = GetPosition( i );
    INDEX start = pos - GetWordInSentence( pos );
    char length = GetSentenceLength( GetSentence( pos ) );
    // print document name
    if (m_useDocument) {
      INDEX sentence = GetSentence( pos );
      INDEX document = GetDocument( sentence );
      PrintDocumentName( document );
      cout << '\t';
    }
    // print sentence
    for(char i=0; i<length; i++) {
      if (i>0) cout << " ";
      cout << GetWord( start + i );
    }
    cout << endl;
  }
}

SuffixArray::INDEX SuffixArray::GetDocument( INDEX sentence ) const
{
  // binary search
  INDEX min = 0;
  INDEX max = m_documentCount-1;
  if (sentence >= m_document[max]) {
    return max;
  }
  while(true) {
    INDEX mid = (min + max) / 2;
    if (sentence >= m_document[mid] && sentence < m_document[mid+1]) {
      return mid;
    }
    if (sentence < m_document[mid]) {
      max = mid-1;
    } else {
      min = mid+1;
    }
  }
}

void SuffixArray::Save(const string& fileName ) const
{
  FILE *pFile = fopen ( fileName.c_str() , "w" );
  if (pFile == NULL) Error("cannot open",fileName);

  fwrite( &m_size, sizeof(INDEX), 1, pFile );
  fwrite( m_array, sizeof(WORD_ID), m_size, pFile ); // corpus
  fwrite( m_index, sizeof(INDEX), m_size, pFile );   // suffix array
  fwrite( m_wordInSentence, sizeof(char), m_size, pFile); // word index
  fwrite( m_sentence, sizeof(INDEX), m_size, pFile); // sentence index

  fwrite( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  fwrite( m_sentenceLength, sizeof(char), m_sentenceCount, pFile); // sentence length

  char useDocument = m_useDocument; // not sure if that is needed
  fwrite( &useDocument, sizeof(char), 1, pFile );
  if (m_useDocument) {
    fwrite( &m_documentCount, sizeof(INDEX), 1, pFile );
    fwrite( m_document, sizeof(INDEX), m_documentCount, pFile );
    fwrite( m_documentName, sizeof(INDEX), m_documentCount, pFile );
    fwrite( &m_documentNameLength, sizeof(INDEX), 1, pFile );
    fwrite( m_documentNameBuffer, sizeof(char), m_documentNameLength, pFile );
  }
  fclose( pFile );

  m_vcb.Save( fileName + ".src-vcb" );
}

void SuffixArray::Load(const string& fileName )
{
  FILE *pFile = fopen ( fileName.c_str() , "r" );
  if (pFile == NULL) Error("no such file or directory", fileName);

  cerr << "loading from " << fileName << endl;

  fread( &m_size, sizeof(INDEX), 1, pFile )
  || Error("could not read m_size from", fileName);
  cerr << "words in corpus: " << m_size << endl;

  m_array = (WORD_ID*) calloc( sizeof( WORD_ID ), m_size );
  m_index = (INDEX*) calloc( sizeof( INDEX ), m_size );
  m_wordInSentence = (char*) calloc( sizeof( char ), m_size );
  m_sentence = (INDEX*) calloc( sizeof( INDEX ), m_size );
  CheckAllocation(m_array != NULL, "m_array");
  CheckAllocation(m_index != NULL, "m_index");
  CheckAllocation(m_wordInSentence != NULL, "m_wordInSentence");
  CheckAllocation(m_sentence != NULL, "m_sentence");
  fread( m_array, sizeof(WORD_ID), m_size, pFile ) // corpus
  || Error("could not read m_array from", fileName);
  fread( m_index, sizeof(INDEX), m_size, pFile )   // suffix array
  || Error("could not read m_index from", fileName);
  fread( m_wordInSentence, sizeof(char), m_size, pFile) // word index
  || Error("could not read m_wordInSentence from", fileName);
  fread( m_sentence, sizeof(INDEX), m_size, pFile ) // sentence index
  || Error("could not read m_sentence from", fileName);

  fread( &m_sentenceCount, sizeof(INDEX), 1, pFile )
  || Error("could not read m_sentenceCount from", fileName);
  cerr << "sentences in corpus: " << m_sentenceCount << endl;

  m_sentenceLength = (char*) calloc( sizeof( char ), m_sentenceCount );
  CheckAllocation(m_sentenceLength != NULL, "m_sentenceLength");
  fread( m_sentenceLength, sizeof(char), m_sentenceCount, pFile) // sentence length
  || Error("could not read m_sentenceLength from", fileName);

  if (m_useDocument) { // do not read it when you do not need it
    char useDocument;
    fread( &useDocument, sizeof(char), 1, pFile )
    || Error("could not read m_useDocument from", fileName);
    if (!useDocument) {
      cerr << "Error: stored suffix array does not have a document index\n";
      exit(1);
    }
    fread( &m_documentCount, sizeof(INDEX), 1, pFile )
    || Error("could not read m_documentCount from", fileName);
    m_document = (INDEX*) calloc( sizeof( INDEX ), m_documentCount );
    m_documentName = (INDEX*) calloc( sizeof( INDEX ), m_documentCount );
    CheckAllocation(m_document != NULL, "m_document");
    CheckAllocation(m_documentName != NULL, "m_documentName");
    fread( m_document, sizeof(INDEX), m_documentCount, pFile )
    || Error("could not read m_document from", fileName);
    fread( m_documentName, sizeof(INDEX), m_documentCount, pFile )
    || Error("could not read m_documentName from", fileName);
    fread( &m_documentNameLength, sizeof(INDEX), 1, pFile )
    || Error("could not read m_documentNameLength from", fileName);
    m_documentNameBuffer = (char*) calloc( sizeof( char ), m_documentNameLength );
    CheckAllocation(m_documentNameBuffer != NULL, "m_documentNameBuffer");
    fread( m_documentNameBuffer, sizeof(char), m_documentNameLength, pFile )
    || Error("could not read m_document from", fileName);
  }

  fclose( pFile );

  m_vcb.Load( fileName + ".src-vcb" );
}

void SuffixArray::CheckAllocation( bool check, const char *dataStructure ) const
{
  if (check) return;
  cerr << "Error: could not allocate memory for " << dataStructure << endl;
  exit(1);
}

bool SuffixArray::Error( const char *message, const string &fileName) const
{
  cerr << "Error: " << message << " " << fileName << endl;
  exit(1);
  return true; // yeah, i know.
}
