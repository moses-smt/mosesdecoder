// $Id$

#ifndef _TABLES_H
#define _TABLES_H

#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <string>
#include <queue>
#include <map>
#include <cmath>

namespace MosesTraining
{

typedef std::string WORD;
typedef unsigned int WORD_ID;

class Vocabulary
{
public:
  std::map<WORD, WORD_ID>  lookup;
  std::vector< WORD > vocab;
  WORD_ID storeIfNew( const WORD& );
  WORD_ID getWordID( const WORD& );
  inline WORD &getWord( const WORD_ID id ) {
    return vocab[ id ];
  }
};

typedef std::vector< WORD_ID > PHRASE;
typedef unsigned int PHRASE_ID;

class PhraseTable
{
public:
  std::map< PHRASE, PHRASE_ID > lookup;
  std::vector< PHRASE > phraseTable;
  PHRASE_ID storeIfNew( const PHRASE& );
  PHRASE_ID getPhraseID( const PHRASE& );
  void clear();
  inline PHRASE &getPhrase( const PHRASE_ID id ) {
    return phraseTable[ id ];
  }
};

typedef std::vector< std::pair< PHRASE_ID, double > > PHRASEPROBVEC;

class TTable
{
public:
  std::map< PHRASE_ID, std::vector< std::pair< PHRASE_ID, double > > > ttable;
  std::map< PHRASE_ID, std::vector< std::pair< PHRASE_ID, std::vector< double > > > > ttableMulti;
};

class DTable
{
public:
  std::map< int, double > dtable;
  void init();
  void load( const std::string& );
  double get( int );
};

}

#endif
