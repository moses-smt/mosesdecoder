// $Id: LanguageModelInternal.h 979 2006-11-16 10:43:40Z nicolabertoldi $

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

#include "LanguageModelSingleFactor.h"
#include "NGramCollection.h"

/** Guaranteed cross-platform LM implementation designed to mimic LM used in regression tests
*/
class LanguageModelInternal : public LanguageModelSingleFactor
{
protected:
	std::vector<const NGramNode*> m_lmIdLookup;
	NGramCollection m_map;

	const NGramNode* GetLmID( const Factor *factor ) const
	{
		size_t factorId = factor->GetId();
		return ( factorId >= m_lmIdLookup.size()) ? NULL : m_lmIdLookup[factorId];        
  };

	float GetValue(const Factor *factor0, State* finalState) const;
	float GetValue(const Factor *factor0, const Factor *factor1, State* finalState) const;
	float GetValue(const Factor *factor0, const Factor *factor1, const Factor *factor2, State* finalState) const;

public:
	LanguageModelInternal(bool registerScore, ScoreIndexManager &scoreIndexManager);
	bool Load(const std::string &filePath
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	float GetValue(const std::vector<const Word> &contextFactor
												, State* finalState = 0
												, unsigned int* len = 0) const;
};

