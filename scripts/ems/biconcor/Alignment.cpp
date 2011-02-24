#include "Alignment.h"
#include <string>
#include <stdlib.h>
#include <cstring>

using namespace std;

void Alignment::Create( string fileName )
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
    vector<string> alignmentSequence = Tokenize( line );
    m_size += alignmentSequence.size();
    m_sentenceCount++;
  }
  textFile.close();
  cerr << m_size << " alignment points" << endl;

  // allocate memory
  m_array = (char*) calloc( sizeof( char ), m_size*2 );
  m_sentenceEnd = (INDEX*) calloc( sizeof( INDEX ), m_sentenceCount );

  // fill the array
  int alignmentPointIndex = 0;
  int sentenceId = 0;
  textFile.open(fileName.c_str());
  fileP = &textFile;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, LINE_MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    vector<string> alignmentSequence = Tokenize( line );
    for(int i=0; i<alignmentSequence.size(); i++) {
      int s,t;
      // cout << "scaning " << alignmentSequence[i].c_str() << endl;
      if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &s, &t)) {
        cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentence " << sentenceId << endl;
      }
      m_array[alignmentPointIndex++] = (char) s;
      m_array[alignmentPointIndex++] = (char) t;
    }
    m_sentenceEnd[ sentenceId++ ] = alignmentPointIndex - 2;
  }
  textFile.close();
  cerr << "done reading " << (alignmentPointIndex/2) << " alignment points, " << sentenceId << " sentences." << endl;
}

Alignment::~Alignment()
{
  free(m_array);
  free(m_sentenceEnd);
}

vector<string> Alignment::Tokenize( const char input[] )
{
  vector< string > token;
  bool betweenWords = true;
  int start=0;
  int i=0;
  for(; input[i] != '\0'; i++) {
    bool isSpace = (input[i] == ' ' || input[i] == '\t');

    if (!isSpace && betweenWords) {
      start = i;
      betweenWords = false;
    } else if (isSpace && !betweenWords) {
      token.push_back( string( input+start, i-start ) );
      betweenWords = true;
    }
  }
  if (!betweenWords)
    token.push_back( string( input+start, i-start ) );
  return token;
}

bool Alignment::PhraseAlignment( INDEX sentence, char target_length,
                                 char source_start, char source_end,
                                 char &target_start, char &target_end,
                                 char &pre_null, char &post_null )
{
  vector< char > alignedTargetWords;

  // get index for first alignment point
  INDEX sentenceStart = 0;
  if (sentence > 0) {
    sentenceStart = m_sentenceEnd[ sentence-1 ] + 2;
  }

  // get target phrase boundaries
  target_start = target_length;
  target_end = 0;
  for(INDEX ap = sentenceStart; ap <= m_sentenceEnd[ sentence ]; ap += 2 ) {
    char source = m_array[ ap ];
    if (source >= source_start && source <= source_end ) {
      char target =  m_array[ ap+1 ];
      if (target < target_start) target_start = target;
      if (target > target_end )  target_end   = target;
    }
  }
  if (target_start == target_length) {
    return false; // done if no alignment points
  }

  // check consistency
  for(INDEX ap = sentenceStart; ap <= m_sentenceEnd[ sentence ]; ap += 2 ) {
    char target =  m_array[ ap+1 ];
    if (target >= target_start && target <= target_end ) {
      char source = m_array[ ap ];
      if (source < source_start || source > source_end) {
        return false; // alignment point out of range
      }
    }
  }

  // create array for unaligned words
  for( int i=0; i<target_length; i++ ) {
    m_unaligned[i] = true;
  }
  for(INDEX ap = sentenceStart; ap <= m_sentenceEnd[ sentence ]; ap += 2 ) {
    char target =  m_array[ ap+1 ];
    m_unaligned[ target ] = false;
  }

  // prior unaligned words
  pre_null = 0;
  for(char target = target_start-1; target >= 0 && m_unaligned[ target ]; target--) {
    pre_null++;
  }

  // post unaligned words;
  post_null = 0;
  for(char target = target_end+1; target < target_length && m_unaligned[ target ]; target++) {
    post_null++;
  }
  return true;
}

void Alignment::Save( string fileName )
{
  FILE *pFile = fopen ( (fileName + ".align").c_str() , "w" );

  fwrite( &m_size, sizeof(INDEX), 1, pFile );
  fwrite( m_array, sizeof(char), m_size*2, pFile ); // corpus

  fwrite( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  fwrite( m_sentenceEnd, sizeof(INDEX), m_sentenceCount, pFile); // sentence index
  fclose( pFile );
}

void Alignment::Load( string fileName )
{
  FILE *pFile = fopen ( (fileName + ".align").c_str() , "r" );
  cerr << "loading from " << fileName << ".align" << endl;

  fread( &m_size, sizeof(INDEX), 1, pFile );
  cerr << "alignment points in corpus: " << m_size << endl;
  m_array = (char*) calloc( sizeof(char), m_size*2 );
  fread( m_array, sizeof(char), m_size*2, pFile ); // corpus

  fread( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  cerr << "sentences in corpus: " << m_sentenceCount << endl;
  m_sentenceEnd = (INDEX*) calloc( sizeof(INDEX), m_sentenceCount );
  fread( m_sentenceEnd, sizeof(INDEX), m_sentenceCount, pFile); // sentence index
  fclose( pFile );
  cerr << "done loading\n";
}
