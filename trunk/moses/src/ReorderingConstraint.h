// $Id: ReorderingConstraint.h 1466 2007-09-27 23:22:58Z redpony $
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2008 University of Edinburgh

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

#include <limits>
#include <vector>
#include <iostream>
#include <cstring>
#include <cmath>
#include "TypeDef.h"

namespace Moses
{

class InputType;

/** vector of boolean used to represent whether a word has been translated or not */
class ReorderingConstraint 
{
	friend std::ostream& operator<<(std::ostream& out, const ReorderingConstraint& reorderingConstraint);
protected:
	// const size_t m_size; /**< number of words in sentence */
	size_t m_size; /**< number of words in sentence */
	bool	*m_bitmap;	/**< flag for each word if it is a wall */

public:

	//! create ReorderingConstraint of length size and initialise to zero
	ReorderingConstraint(size_t size)
		:m_size	(size)
	{
		m_bitmap = (bool*) malloc(sizeof(bool) * size);

		for (size_t pos = 0 ; pos < m_size ; pos++)
		{
			m_bitmap[pos] = false;
		}
	}

	~ReorderingConstraint()
	{
		free(m_bitmap);
	}


	//! whether a word has been translated at a particular position
	bool GetWall(size_t pos) const
	{
		return m_bitmap[pos];
	}

	//! set value at a particular position
	void SetValue( size_t pos, bool value )
	{
		m_bitmap[pos] = value;
	}

	//! set the reordering wall based on the words in the sentence
	void SetWall( const InputType& sentence );

	//! checks if there is a wall in the interval [start,end]
	bool ContainsWall( size_t start, size_t end ) const;

};

}
