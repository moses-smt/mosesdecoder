// $Id: NGramCollection.h 2939 2010-02-24 11:15:44Z jfouet $

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

#ifndef moses_NGramCollection_h
#define moses_NGramCollection_h

#include <map>
#include <vector>
#include "NGramNode.h"

namespace Moses
{

class Factor;

typedef std::vector<const Factor*> FactorVector;

class NGramCollection
{
protected:
	typedef std::map<const Factor*, NGramNode*> Collection;
	Collection m_collection;

	void Add(const Factor *factor, const NGramNode &ngramNode);
public:
	NGramCollection()
	{
	}
	~NGramCollection();

	NGramNode *GetOrCreateNGram(const Factor *factor);
	NGramNode *GetNGram(const Factor *factor);
	const NGramNode *GetNGram(const Factor *factor) const;

};

}

#endif
