// $Id: tables-core.h 1470 2007-10-02 21:43:54Z redpony $

#pragma once

#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <map>
#include <cmath>

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

namespace tmmt
{

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

typedef std::string WORD;
typedef unsigned int WORD_ID;

class Vocabulary
{
public:
  std::map<WORD, WORD_ID> lookup;
  std::vector< WORD > vocab;
  WORD_ID StoreIfNew( const WORD& );
  WORD_ID GetWordID( const WORD& );
  std::vector<WORD_ID> Tokenize( const char[] );
  inline WORD &GetWord( WORD_ID id ) const {
    WORD &i = (WORD&) vocab[ id ];
    return i;
  }

protected:
#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif


};

}

