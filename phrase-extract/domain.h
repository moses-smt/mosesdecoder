// $Id$

#ifndef _DOMAIN_H
#define _DOMAIN_H

#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <map>
#include <cmath>

extern std::vector<std::string> tokenize( const char*);

namespace MosesTraining
{

class Domain
{
public:
  std::vector< std::pair< int, std::string > > spec;
  std::vector< std::string > list;
  std::map< std::string, int > name2id;
  void load( const std::string &fileName );
  std::string getDomainOfSentence( int sentenceId );
};

}

#endif
