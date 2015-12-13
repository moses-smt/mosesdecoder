// $Id: Vocabulary.cpp 1565 2008-02-22 14:42:01Z bojar $
#include "Vocabulary.h"
#ifdef WITH_THREADS
#include <boost/thread/locks.hpp>
#endif

using namespace std;

namespace tmmt
{

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

  {
    // read=lock scope
#ifdef WITH_THREADS
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif
    map<WORD, WORD_ID>::iterator i = lookup.find( word );

    if( i != lookup.end() )
      return i->second;
  }

#ifdef WITH_THREADS
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
  WORD_ID id = vocab.size();
  vocab.push_back( word );
  lookup[ word ] = id;
  return id;
}

WORD_ID Vocabulary::GetWordID( const WORD &word )
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif
  map<WORD, WORD_ID>::iterator i = lookup.find( word );
  if( i == lookup.end() )
    return 0;
  WORD_ID w= (WORD_ID) i->second;
  return w;
}

}

