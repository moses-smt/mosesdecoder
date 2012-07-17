// $Id$
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "NonTerminal.h"
#include "Word.h"
#include "WordsRange.h"

namespace Moses
{
class Word;

/** base class of the collection that contains all cells for hierarchical/syntax decoding
 *  @todo check whether this is still needed. It was required when chart decoding was in 
 *  a separate lib but that's not the case anymore
 */
class CellCollection
{
public:
  virtual ~CellCollection()
  {}
};

}

