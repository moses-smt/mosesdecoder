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

#include <map>
#include <string>
#include <stdexcept>
#include "Phrase.h"

class StaticData;

class WordDeletionTable
{
	typedef float COST_TYPE;
	
	protected:
	
		std::map<Phrase, COST_TYPE> m_deletionCosts; //map each source-language phrase to the cost of deleting it
		
	public:
	
		/***
		 * should only be called once for a given instance
		 */
		void Load(const std::string& filename, StaticData& staticData);
		
		/***
		 * \throw invalid_argument if the given phrase isn't in our table
		 */
		COST_TYPE GetDeletionCost(const Phrase& sourcePhrase) const throw(std::invalid_argument)
		{
			std::cout << "WordDeletionTable::GetDeletionCost()" << std::endl;
			std::map<Phrase, COST_TYPE>::const_iterator i = m_deletionCosts.find(sourcePhrase);
			if(i == m_deletionCosts.end())
				throw std::invalid_argument("WordDeletionTable::GetDeletionCost()");
			return i->second;
		}
};
