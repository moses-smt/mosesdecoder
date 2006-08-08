//#include "beammain.h"
#include "tables-core.h"

#define TABLE_LINE_MAX_LENGTH 1000
#define UNKNOWNSTR	"UNK"

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) {_IS.getline(_LINE, _SIZE, _DELIM); if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();}

// as in beamdecoder/tables.cpp
vector<string> tokenize( char input[] ) {
  vector< string > token;
  bool betweenWords = true;
  int start;
  int i=0;
  for(; input[i] != '\0'; i++) {
    bool isSpace = (input[i] == ' ' || input[i] == '\t');

    if (!isSpace && betweenWords) {
      start = i;
      betweenWords = false;
    }
    else if (isSpace && !betweenWords) {
      token.push_back( string( input+start, i-start ) );
      betweenWords = true;
    }
  }
  if (!betweenWords)
    token.push_back( string( input+start, i-start+1 ) );
  return token;
}

WORD_ID Vocabulary::storeIfNew( WORD word ) {
  if( lookup.find( word ) != lookup.end() )
    return lookup[ word ];

  WORD_ID id = vocab.size();
  vocab.push_back( word );
  lookup[ word ] = id;
  return id;  
}

WORD_ID Vocabulary::getWordID( WORD word ) {
  if( lookup.find( word ) == lookup.end() )
    return 0;
  return lookup[ word ];
}

PHRASE_ID PhraseTable::storeIfNew( PHRASE phrase ) {
  if( lookup.find( phrase ) != lookup.end() )
    return lookup[ phrase ];

  PHRASE_ID id  = phraseTable.size();
  phraseTable.push_back( phrase );
  lookup[ phrase ] = id;
  return id;
}

PHRASE_ID PhraseTable::getPhraseID( PHRASE phrase ) {
  if( lookup.find( phrase ) == lookup.end() )
    return 0;
  return lookup[ phrase ];
}

void PhraseTable::clear() {
  lookup.clear();
  phraseTable.clear();
}

void DTable::init() {
  for(int i = -10; i<10; i++)
    dtable[i] = -abs( i );
}

void DTable::load( string fileName ) {
  ifstream inFile;
  inFile.open(fileName.c_str());
  istream *inFileP = &inFile;

  char line[TABLE_LINE_MAX_LENGTH];
  int i=0;
  while(true) {
    i++;
    SAFE_GETLINE((*inFileP), line, TABLE_LINE_MAX_LENGTH, '\n');
    if (inFileP->eof()) break;

    vector<string> token = tokenize( line );
    if (token.size() < 2) {
      cerr << "line " << i << " in " << fileName << " too short, skipping\n";
      continue;
    }

    int d = atoi( token[0].c_str() );
    double prob = log( atof( token[1].c_str() ) );
    dtable[ d ] = prob;
  }  
}

double DTable::get( int distortion ) {
  if (dtable.find( distortion ) == dtable.end())
    return log( 0.00001 );
  return dtable[ distortion ];
}
