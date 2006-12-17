// $Id: PhraseAlignment.h 988 2006-11-21 19:35:37Z hieuhoang1972 $
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
#include <iterator>
#include "TypeDef.h"
#include "Util.h"

typedef std::vector<size_t> AlignVec;
typedef std::vector<AlignVec> PhraseAlignVec;
typedef std::back_insert_iterator<PhraseAlignVec> AlignInserter;

class PhraseAlignment
{
	friend std::ostream& operator<<(std::ostream&, const PhraseAlignment&);

protected:
	PhraseAlignVec m_sourceAlign, m_targetAlign;

public:
	PhraseAlignment()
	{}
	AlignInserter GetInserter(FactorDirection direction);

	TO_STRING();
};

