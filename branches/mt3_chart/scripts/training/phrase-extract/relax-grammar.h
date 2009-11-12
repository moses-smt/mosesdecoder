// $Id: relax-grammar.h 0 2009-09-18 01:00:00Z init $
// vim:tabstop=2

/***********************************************************************
  Moses - factored hierarchical phrase-based language decoder
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

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include "SyntaxTree.h"
#include "XmlTree.h"

using namespace std;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
		_IS.getline(_LINE, _SIZE, _DELIM);																	 \
		if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();							 \
		if (_IS.gcount() == _SIZE-1) {																			 \
			cerr << "Line too long! Buffer overflow. Delete lines >="	<< _SIZE \
					 << " chars or raise LINE_MAX_LENGTH in relax-grammar"				 \
					 << endl;																											 \
			exit(1);																													 \
		}																																		 \
	}
#define LINE_MAX_LENGTH 60000

// tokenize is coded in tables-core.cpp
vector<string> tokenize( const char [] );

bool leftBinarizeFlag = false;
bool rightBinarizeFlag = false;
char SAMTLevel = 0;

// functions
void init(int argc, char* argv[]);
void store( SyntaxTree &tree, vector<string> &words, ofstream &outFile );
void LeftBinarize( SyntaxTree &tree, ParentNodes &parents );
void RightBinarize( SyntaxTree &tree, ParentNodes &parents );
void SAMT( SyntaxTree &tree, ParentNodes &parents );

