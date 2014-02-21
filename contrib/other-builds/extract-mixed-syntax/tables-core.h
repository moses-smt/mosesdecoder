#pragma once
// $Id: tables-core.h 2416 2009-07-30 11:07:38Z hieuhoang1972 $

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

vector<string> tokenize( const char[] );

//! delete and remove every element of a collection object such as map, set, list etc
template<class COLL>
void RemoveAllInColl(COLL &coll)
{
	for (typename COLL::const_iterator iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		delete (*iter);
	}
	coll.clear();
}

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


