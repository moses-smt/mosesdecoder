#include "TargetCorpus.h"
#include <string>
#include <stdlib.h>
#include <cstring>

void TargetCorpus::Create( string fileName )
{
  ifstream textFile;
  char line[LINE_MAX_LENGTH];

  // count the number of words first;
  textFile.open(fileName.c_str());
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

  // fill the array
  int wordIndex = 0;
  int sentenceId = 0;
  textFile.open(fileName.c_str());
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

TargetCorpus::~TargetCorpus()
{
  free(m_array);
  free(m_sentenceEnd);
}

WORD TargetCorpus::GetWordFromId( const WORD_ID id ) const
{
  return m_vcb.GetWord( id );
}

WORD TargetCorpus::GetWord( INDEX sentence, char word )
{
  return m_vcb.GetWord( GetWordId( sentence, word ) );
}

WORD_ID TargetCorpus::GetWordId( INDEX sentence, char word )
{
  if (sentence == 0) {
    return m_array[ word ];
  }
  return m_array[ m_sentenceEnd[ sentence-1 ] + 1 + word ] ;
}

char TargetCorpus::GetSentenceLength( INDEX sentence )
{
  if (sentence == 0) {
    return (char) m_sentenceEnd[ 0 ]+1;
  }
  return (char) ( m_sentenceEnd[ sentence ] - m_sentenceEnd[ sentence-1 ] );
}

void TargetCorpus::Save( string fileName )
{
  FILE *pFile = fopen ( (fileName + ".tgt").c_str() , "w" );

  fwrite( &m_size, sizeof(INDEX), 1, pFile );
  fwrite( m_array, sizeof(WORD_ID), m_size, pFile ); // corpus

  fwrite( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  fwrite( m_sentenceEnd, sizeof(INDEX), m_sentenceCount, pFile); // sentence index
  fclose( pFile );

  m_vcb.Save( fileName + ".tgt-vcb" );
}

void TargetCorpus::Load( string fileName )
{
  FILE *pFile = fopen ( (fileName + ".tgt").c_str() , "r" );
  cerr << "loading from " << fileName << ".tgt" << endl;

  fread( &m_size, sizeof(INDEX), 1, pFile );
  cerr << "words in corpus: " << m_size << endl;
  m_array = (WORD_ID*) calloc( sizeof(WORD_ID), m_size );
  fread( m_array, sizeof(WORD_ID), m_size, pFile ); // corpus

  fread( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  cerr << "sentences in corpus: " << m_sentenceCount << endl;
  m_sentenceEnd = (INDEX*) calloc( sizeof(INDEX), m_sentenceCount );
  fread( m_sentenceEnd, sizeof(INDEX), m_sentenceCount, pFile); // sentence index
  fclose( pFile );
  m_vcb.Load( fileName + ".tgt-vcb" );
}

