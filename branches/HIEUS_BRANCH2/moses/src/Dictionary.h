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

#include <vector>
#include "FactorTypeSet.h"

class Dictionary
{
protected:
	static size_t s_index;
	
	const size_t m_noScoreComponent, m_index;
	std::vector< FactorTypeSet* > m_factorsUsed;

public:
	Dictionary(size_t noScoreComponent);

	const FactorTypeSet &GetFactorsUsed(FactorDirection direction) const
	{
		assert(static_cast<size_t>(direction)<m_factorsUsed.size());
		assert(m_factorsUsed[direction]);
		return *m_factorsUsed[direction];
	}
	virtual ~Dictionary();

	size_t GetNoScoreComponent() const
	{
		return m_noScoreComponent;
	}
	size_t GetIndex() const
	{
		return m_index;
	}
	virtual DecodeType GetDecodeType() const = 0;

	// clean up temporary memory, called after processing each sentence
	virtual void CleanUp();
};
