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

#define LINE_MAX_LENGTH 1000000

bool leftBinarizeFlag = false;
bool rightBinarizeFlag = false;
char SAMTLevel = 0;

// functions
void init(int argc, char* argv[]);
void store( MosesTraining::SyntaxTree &tree, std::vector<std::string> &words );
void LeftBinarize( MosesTraining::SyntaxTree &tree, MosesTraining::ParentNodes &parents );
void RightBinarize( MosesTraining::SyntaxTree &tree, MosesTraining::ParentNodes &parents );
void SAMT( MosesTraining::SyntaxTree &tree, MosesTraining::ParentNodes &parents );

