// $Id: NGramCollection.cpp,v 1.2 2006/06/14 18:27:55 h.hoang Exp $

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

#include "NGramCollection.h"
#include "NGramNode.h"

NGramCollection::~NGramCollection()
{
	Collection::iterator iter;
	for (iter = m_collection.begin() ; iter != m_collection.end() ; ++iter)
	{
		delete (iter->second);
	}
}

void NGramCollection::Add(FACTOR_ID factor, const NGramNode &ngramNode)
{
}

NGramNode *NGramCollection::GetOrCreateNGram(FACTOR_ID factor)
{
	Collection::iterator iter = m_collection.find(factor);
	if (iter == m_collection.end())
	{
		return (m_collection[factor] = new NGramNode());
	}
	else
	{
		return (iter->second);
	}
}

NGramNode *NGramCollection::GetNGram(FACTOR_ID factor)
{
	Collection::iterator iter = m_collection.find(factor);
	return (iter == m_collection.end()) ? NULL : (iter->second) ;
}

const NGramNode *NGramCollection::GetNGram(FACTOR_ID factor) const
{
	Collection::const_iterator iter = m_collection.find(factor);
	return (iter == m_collection.end()) ? NULL : (iter->second) ;
}

