// $Id$

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

#define TABLE_LINE_MAX_LENGTH 1000
#define UNKNOWNSTR	"UNK"

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
                _IS.getline(_LINE, _SIZE, _DELIM); \
                if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear(); \
                if (_IS.gcount() == _SIZE-1) { \
                  cerr << "Line too long! Buffer overflow. Delete lines >=" \
                    << _SIZE << " chars or raise TABLE_LINE_MAX_LENGTH in phrase-extract/tables-core.cpp" \
                    << endl; \
                    exit(1); \
                } \
              }


vector<string> tokenize( const char[] );

typedef string WORD;
typedef unsigned int WORD_ID;

class Vocabulary {
 public:
  map<WORD, WORD_ID>  lookup;
  vector< WORD > vocab;
  WORD_ID storeIfNew( const WORD& );
  WORD_ID getWordID( const WORD& );
  inline WORD &getWord( WORD_ID id ) const { WORD &i = (WORD&) vocab[ id ]; return i; }
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
