/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#pragma once

#include <string>
#include <map>

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

