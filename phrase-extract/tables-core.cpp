// $Id$
//#include "beammain.h"
#include "tables-core.h"

#define TABLE_LINE_MAX_LENGTH 1000
#define UNKNOWNSTR	"UNK"

using namespace std;

// as in beamdecoder/tables.cpp
vector<string> tokenize( const char* input )
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

namespace MosesTraining
{

bool isNonTerminal( const WORD &symbol )
{
  return symbol.substr(0, 1) == "[" && symbol.substr(symbol.size()-1, 1) == "]";
}

WORD_ID Vocabulary::storeIfNew( const WORD& word )
{
  map<WORD, WORD_ID>::iterator i = lookup.find( word );

  if( i != lookup.end() )
    return i->second;

  WORD_ID id = vocab.size();
  vocab.push_back( word );
  lookup[ word ] = id;
  return id;
}

WORD_ID Vocabulary::getWordID( const WORD& word )
{
  map<WORD, WORD_ID>::iterator i = lookup.find( word );
  if( i == lookup.end() )
    return 0;
  return i->second;
}

PHRASE_ID PhraseTable::storeIfNew( const PHRASE& phrase )
{
  map< PHRASE, PHRASE_ID >::iterator i = lookup.find( phrase );
  if( i != lookup.end() )
    return i->second;

  PHRASE_ID id  = phraseTable.size();
  phraseTable.push_back( phrase );
  lookup[ phrase ] = id;
  return id;
}

PHRASE_ID PhraseTable::getPhraseID( const PHRASE& phrase )
{
  map< PHRASE, PHRASE_ID >::iterator i = lookup.find( phrase );
  if( i == lookup.end() )
    return 0;
  return i->second;
}

void PhraseTable::clear()
{
  lookup.clear();
  phraseTable.clear();
}

void DTable::init()
{
  for(int i = -10; i<10; i++)
    dtable[i] = -abs( i );
}

void DTable::load( const string& fileName )
{
  ifstream inFile;
  inFile.open(fileName.c_str());

  std::string line;
  int i=0;
  while(true) {
    i++;
    getline(inFile, line);
    if (inFile.eof()) break;
    if (!inFile) {
      std::cerr << "Error reading from " << fileName << std::endl;
      abort();
    }

    vector<string> token = tokenize(line.c_str());
    if (token.size() < 2) {
      cerr << "line " << i << " in " << fileName << " too short, skipping\n";
      continue;
    }

    int d = atoi( token[0].c_str() );
    double prob = log( atof( token[1].c_str() ) );
    dtable[ d ] = prob;
  }
}

double DTable::get( int distortion )
{
  if (dtable.find( distortion ) == dtable.end())
    return log( 0.00001 );
  return dtable[ distortion ];
}

}

