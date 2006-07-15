// $Id$

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

#include "TypeDef.h"
#include "Util.h"

class SquareMatrix
{
protected:
	const size_t m_size;
	float *m_array;

	SquareMatrix(); // not implemented
	SquareMatrix(const SquareMatrix &copy); // not implemented
	
public:
	SquareMatrix(size_t size)
	:m_size(size)
	{
		m_array = (float*) malloc(sizeof(float) * size * size);
	}
	~SquareMatrix()
	{
		free(m_array);
	}
	inline float GetScore(size_t row, size_t col) const
	{
		return m_array[row * m_size + col];
	}
	inline void SetScore(size_t row, size_t col, float value)
	{
		m_array[row * m_size + col] = value;
	}
	
};

