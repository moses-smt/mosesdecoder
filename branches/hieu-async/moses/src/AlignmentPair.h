// $Id: AlignmentPair.h 988 2006-11-21 19:35:37Z hieuhoang1972 $
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
#include "AlignmentPhrase.h"

typedef std::back_insert_iterator<AlignmentPhrase> AlignmentPairInserter;

/** represent the alignment info between source and target phrase */
class AlignmentPair
{
	friend std::ostream& operator<<(std::ostream&, const AlignmentPair&);

protected:
	AlignmentPhrase m_sourceAlign, m_targetAlign;

public:
	AlignmentPair()
	{}

	/** get the back_insert_iterator to the source or target alignment vector so that
		*	they could be populated
		*/
	AlignmentPairInserter GetInserter(FactorDirection direction);
	const AlignmentPhrase &GetPhraseAlignVec(FactorDirection direction) const
	{
		return (direction == Input) ? m_sourceAlign : m_targetAlign;

	}

	/** used by the unknown word handler.
		* Set alignment to 0
		*/
	void SetAlignment();

	TO_STRING();
};

