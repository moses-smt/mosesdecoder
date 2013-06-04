#include "TargetCorpus.h"

#include <fstream>
#include <string>
#include <stdlib.h>
#include <cstring>

namespace
{

const int LINE_MAX_LENGTH = 10000;

} // namespace

using namespace std;

TargetCorpus::TargetCorpus()
  : m_array(NULL),
    m_sentenceEnd(NULL),
    m_vcb(),
    m_size(0),
    m_sentenceCount(0) {}

TargetCorpus::~TargetCorpus()
{
  free(m_array);
  free(m_sentenceEnd);
}

void TargetCorpus::Create(const string& fileName )
{
  ifstream textFile;
  char line[LINE_MAX_LENGTH];

  // count the number of words first;
  textFile.open(fileName.c_str());

  if (!textFile) {
    cerr << "no such file or directory " << fileName << endl;
    exit(1);
  }

  istream *fileP = &textFile;
  m_size = 0;
  m_sentenceCount = 0;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, LINE_MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    vector< WORD_ID > words = m_vcb.Tokenize( line );
    m_size += words.size();
    m_sentenceCount++;
  }
  textFile.close();
  cerr << m_size << " words" << endl;

  // allocate memory
  m_array = (WORD_ID*) calloc( sizeof( WORD_ID ), m_size );
  m_sentenceEnd = (INDEX*) calloc( sizeof( INDEX ), m_sentenceCount );

  if (m_array == NULL) {
    cerr << "cannot allocate memory to m_array" << endl;
    exit(1);
  }

  if (m_sentenceEnd == NULL) {
    cerr << "cannot allocate memory to m_sentenceEnd" << endl;
    exit(1);
  }

  // fill the array
  int wordIndex = 0;
  int sentenceId = 0;
  textFile.open(fileName.c_str());

  if (!textFile) {
    cerr << "no such file or directory " << fileName << endl;
    exit(1);
  }

  fileP = &textFile;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, LINE_MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    vector< WORD_ID > words = m_vcb.Tokenize( line );
    vector< WORD_ID >::const_iterator i;

    for( i=words.begin(); i!=words.end(); i++) {
      m_array[ wordIndex++ ] = *i;
    }
    m_sentenceEnd[ sentenceId++ ] = wordIndex-1;
  }
  textFile.close();
  cerr << "done reading " << wordIndex << " words, " << sentenceId << " sentences." << endl;
}

WORD TargetCorpus::GetWordFromId( const WORD_ID id ) const
{
  return m_vcb.GetWord( id );
}

WORD TargetCorpus::GetWord( INDEX sentence, int word ) const
{
  return m_vcb.GetWord( GetWordId( sentence, word ) );
}

WORD_ID TargetCorpus::GetWordId( INDEX sentence, int word ) const
{
  if (sentence == 0) {
    return m_array[ word ];
  }
  return m_array[ m_sentenceEnd[ sentence-1 ] + 1 + word ] ;
}

char TargetCorpus::GetSentenceLength( INDEX sentence ) const
{
  if (sentence == 0) {
    return (char) m_sentenceEnd[ 0 ]+1;
  }
  return (char) ( m_sentenceEnd[ sentence ] - m_sentenceEnd[ sentence-1 ] );
}

void TargetCorpus::Save(const string& fileName ) const
{
  FILE *pFile = fopen ( (fileName + ".tgt").c_str() , "w" );
  if (pFile == NULL) {
    cerr << "Cannot open " << fileName << endl;
    exit(1);
  }

  fwrite( &m_size, sizeof(INDEX), 1, pFile );
  fwrite( m_array, sizeof(WORD_ID), m_size, pFile ); // corpus

  fwrite( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  fwrite( m_sentenceEnd, sizeof(INDEX), m_sentenceCount, pFile); // sentence index
  fclose( pFile );

  m_vcb.Save( fileName + ".tgt-vcb" );
}

void TargetCorpus::Load(const string& fileName )
{
  FILE *pFile = fopen ( (fileName + ".tgt").c_str() , "r" );
  if (pFile == NULL) {
    cerr << "Cannot open " << fileName << endl;
    exit(1);
  }

  cerr << "loading from " << fileName << ".tgt" << endl;

  fread( &m_size, sizeof(INDEX), 1, pFile );
  cerr << "words in corpus: " << m_size << endl;
  m_array = (WORD_ID*) calloc( sizeof(WORD_ID), m_size );

  if (m_array == NULL) {
    cerr << "cannot allocate memory to m_array" << endl;
    exit(1);
  }

  fread( m_array, sizeof(WORD_ID), m_size, pFile ); // corpus

  fread( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  cerr << "sentences in corpus: " << m_sentenceCount << endl;
  m_sentenceEnd = (INDEX*) calloc( sizeof(INDEX), m_sentenceCount );

  if (m_sentenceEnd == NULL) {
    cerr << "cannot allocate memory to m_sentenceEnd" << endl;
    exit(1);
  }

  fread( m_sentenceEnd, sizeof(INDEX), m_sentenceCount, pFile); // sentence index
  fclose( pFile );
  m_vcb.Load( fileName + ".tgt-vcb" );
}
