// $Id: tables-core.h 1470 2007-10-02 21:43:54Z redpony $

#pragma once

#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <string>
#include <queue>
#include <map>
#include <cmath>

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

namespace tmmt
{
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

