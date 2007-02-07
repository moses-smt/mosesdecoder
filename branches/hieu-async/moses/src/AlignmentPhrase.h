// $Id: AlignmentPhrase.h 988 2006-11-21 19:35:37Z hieuhoang1972 $
/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#include <iostream>
#include <vector>
#include "AlignmentElement.h"

class WordsRange;


//! alignments of each word in a phrase
class AlignmentPhrase : public std::vector<AlignmentElement>
{
	friend std::ostream& operator<<(std::ostream& out, const AlignmentPhrase &alignmentPhrase);

public:
	bool IsCompatible(const AlignmentPhrase &compare, size_t startPosCompare) const;
	void Merge(const AlignmentPhrase &newAlignment, const WordsRange &newAlignmentRange);
};



