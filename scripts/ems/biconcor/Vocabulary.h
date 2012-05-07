// $Id: tables-core.h 1470 2007-10-02 21:43:54Z redpony $

#pragma once

#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
                _IS.getline(_LINE, _SIZE, _DELIM); \
                if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear(); \
                if (_IS.gcount() == _SIZE-1) { \
                  std::cerr << "Line too long! Buffer overflow. Delete lines >=" \
                    << _SIZE << " chars or raise MAX_LENGTH in phrase-extract/tables-core.cpp" \
                       << std::endl; \
                  std::exit(1);  \
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
  WORD_ID GetWordID( const WORD& ) const;
  std::vector<WORD_ID> Tokenize( const char[] );
  inline WORD &GetWord( WORD_ID id ) const {
    WORD &i = (WORD&) vocab[ id ];
    return i;
  }
  void Save(const std::string& fileName ) const;
  void Load(const std::string& fileName );
};
