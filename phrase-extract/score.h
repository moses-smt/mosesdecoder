#pragma once
/*
 *  score.h
 *  extract
 *
 *  Created by Hieu Hoang on 28/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <vector>

namespace MosesTraining
{
class LexicalTable
{
public:
  std::map< WORD_ID, std::map< WORD_ID, double > > ltable;
  void load( const std::string &filePath );
  double permissiveLookup( WORD_ID wordS, WORD_ID wordT ) {
    // cout << endl << vcbS.getWord( wordS ) << "-" << vcbT.getWord( wordT ) << ":";
    if (ltable.find( wordS ) == ltable.end()) return 1.0;
    if (ltable[ wordS ].find( wordT ) == ltable[ wordS ].end()) return 1.0;
    // cout << ltable[ wordS ][ wordT ];
    return ltable[ wordS ][ wordT ];
  }
};

// other functions *********************************************
inline bool isNonTerminal( const std::string &word )
{
  return (word.length()>=3 && word[0] == '[' && word[word.length()-1] == ']');
}


}

