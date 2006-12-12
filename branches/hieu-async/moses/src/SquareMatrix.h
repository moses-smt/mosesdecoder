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

#include <iostream>
#include "TypeDef.h"
#include "Util.h"

//! A square array of floats to store future costs
template<typename T>
class SquareMatrix
{
protected:
	const size_t m_size;
	T *m_array;

	SquareMatrix(); // not implemented
	SquareMatrix(const SquareMatrix &copy); // not implemented
	
public:
	SquareMatrix(size_t size)
	:m_size(size)
	{
		m_array = (T*) malloc(sizeof(T) * size * size);
	}
	~SquareMatrix()
	{
		free(m_array);
	}
	inline size_t GetSize() const
	{
		return m_size;
	}
	inline T GetScore(size_t startPos, size_t endPos) const
	{
		assert(startPos <= endPos);
		return m_array[startPos * m_size + endPos];
	}
	inline void SetScore(size_t startPos, size_t endPos, T value)
	{
		assert(startPos <= endPos);
		m_array[startPos * m_size + endPos] = value;
	}
	inline void AddScore(size_t startPos, size_t endPos, T value)
	{
		assert(startPos <= endPos);
		m_array[startPos * m_size + endPos] += value;
	}
	inline void ResetScore(T value)
	{
		for(size_t startPos=0; startPos<m_size; startPos++) {
			for(size_t endPos=startPos; endPos<m_size; endPos++) {
				m_array[startPos * m_size + endPos] = value;
			}
		}
	}
	
	TO_STRING();
};


template<typename T>
inline std::ostream& operator<<(std::ostream &out, const SquareMatrix<T> &matrix)
{
	for (size_t endPos = 0 ; endPos < matrix.GetSize() ; endPos++)
	{
		for (size_t startPos = 0 ; startPos < matrix.GetSize() ; startPos++)
			out << matrix.GetScore(startPos, endPos) << " ";
		out << std::endl;
	}
	
	return out;
}

template<typename T>
TO_STRING_BODY(SquareMatrix<T>);
