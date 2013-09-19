// $Id: Vocabulary.h,v 1.1.1.1 2013/01/06 16:54:13 braunefe Exp $

#pragma once

#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <map>
#include <cmath>

using namespace std;

#define MAX_LENGTH 10000

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
                _IS.getline(_LINE, _SIZE, _DELIM); \
                if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear(); \
                if (_IS.gcount() == _SIZE-1) { \
                  cerr << "Line too long! Buffer overflow. Delete lines >=" \
                    << _SIZE << " chars or raise MAX_LENGTH in phrase-extract/tables-core.cpp" \
                    << endl; \
                    exit(1); \
                } \
              }

typedef string WORD;
typedef unsigned int WORD_ID;

class Vocabulary
{
public:
  map<WORD, WORD_ID> lookup;
  vector< WORD > vocab;
  WORD_ID StoreIfNew( const WORD& );
  WORD_ID GetWordID( const WORD& );
  vector<WORD_ID> Tokenize( const char[] );
  inline WORD &GetWord( WORD_ID id ) const {
    WORD &i = (WORD&) vocab[ id ];
    return i;
  }
  void Save( string fileName );
  void Load( string fileName );
};
