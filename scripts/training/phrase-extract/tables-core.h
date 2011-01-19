// $Id: tables-core.h 3126 2010-04-12 15:22:50Z pjwilliams $

#ifndef _TABLES_H
#define _TABLES_H

#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <map>
#include <cmath>

using namespace std;

extern vector<string> tokenize( const char*);

typedef string WORD;
typedef unsigned int WORD_ID;

class Vocabulary {
 public:
  map<WORD, WORD_ID>  lookup;
  vector< WORD > vocab;
  WORD_ID storeIfNew( const WORD& );
  WORD_ID getWordID( const WORD& );
  inline WORD &getWord( WORD_ID id ) { return vocab[ id ]; }
};

typedef vector< WORD_ID > PHRASE;
typedef unsigned int PHRASE_ID;

class PhraseTable {
 public:
  map< PHRASE, PHRASE_ID > lookup;
  vector< PHRASE > phraseTable;
  PHRASE_ID storeIfNew( const PHRASE& );
  PHRASE_ID getPhraseID( const PHRASE& );
  void clear();
  inline PHRASE &getPhrase( const PHRASE_ID id ) { return phraseTable[ id ]; }
};

typedef vector< pair< PHRASE_ID, double > > PHRASEPROBVEC;

class TTable {
 public:
  map< PHRASE_ID, vector< pair< PHRASE_ID, double > > > ttable;
  map< PHRASE_ID, vector< pair< PHRASE_ID, vector< double > > > > ttableMulti;
};

class DTable {
 public:
  map< int, double > dtable;
  void init();
  void load( const string& );
  double get( int );
};

#endif
