// $Id: Vocabulary.cpp,v 1.1.1.1 2013/01/06 16:54:13 braunefe Exp $
#include "Vocabulary.h"

// as in beamdecoder/tables.cpp
vector<WORD_ID> Vocabulary::Tokenize( const char input[] )
{
  vector< WORD_ID > token;
  bool betweenWords = true;
  int start=0;
  int i=0;
  for(; input[i] != '\0'; i++) {
    bool isSpace = (input[i] == ' ' || input[i] == '\t');

    if (!isSpace && betweenWords) {
      start = i;
      betweenWords = false;
    } else if (isSpace && !betweenWords) {
      token.push_back( StoreIfNew ( string( input+start, i-start ) ) );
      betweenWords = true;
    }
  }
  if (!betweenWords)
    token.push_back( StoreIfNew ( string( input+start, i-start ) ) );
  return token;
}

WORD_ID Vocabulary::StoreIfNew( const WORD& word )
{
  map<WORD, WORD_ID>::iterator i = lookup.find( word );

  if( i != lookup.end() )
    return i->second;

  WORD_ID id = vocab.size();
  vocab.push_back( word );
  lookup[ word ] = id;
  return id;
}

WORD_ID Vocabulary::GetWordID( const WORD &word )
{
  map<WORD, WORD_ID>::iterator i = lookup.find( word );
  if( i == lookup.end() )
    return 0;
  WORD_ID w= (WORD_ID) i->second;
  return w;
}

void Vocabulary::Save( string fileName )
{
  ofstream vcbFile;
  vcbFile.open( fileName.c_str(), ios::out | ios::ate | ios::trunc);
  vector< WORD >::iterator i;
  for(i = vocab.begin(); i != vocab.end(); i++) {
    const string &word = *i;
    vcbFile << word << endl;
  }
  vcbFile.close();
}

void Vocabulary::Load( string fileName )
{
  ifstream vcbFile;
  char line[MAX_LENGTH];
  vcbFile.open(fileName.c_str());
  cerr << "loading from " << fileName << endl;
  istream *fileP = &vcbFile;
  int count = 0;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    int length = 0;
    for(; line[length] != '\0'; length++);
    StoreIfNew( string( line, length ) );
    count++;
  }
  vcbFile.close();
  cerr << count << " word read, vocabulary size " << vocab.size() << endl;
}
