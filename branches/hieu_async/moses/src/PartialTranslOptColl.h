// $Id: PartialTranslOptColl.h 168 2007-10-26 16:05:15Z hieu $

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

#include <list>
#include <iostream>
#include "TranslationOption.h"
#include "Util.h"
#include "StaticData.h"
#include "FactorMask.h"

/** Contains partial translation options, while these are constructed in the class TranslationOption.
 *  The factored translation model allows for multiple translation and 
 *  generation steps during a single Hypothesis expansion. For efficiency, 
 *  all these expansions are precomputed and stored as TranslationOption.
 *  The expansion process itself may be still explode, so efficient handling
 *  of partial translation options during expansion is required. 
 *  This class assists in this tasks by implementing pruning. 
 *  This implementation is similar to the one in HypothesisStack. */

class PartialTranslOptColl
{
	friend std::ostream& operator<<(std::ostream& out, const PartialTranslOptColl &coll);

 protected:
 	typedef std::vector<TranslationOption*> CollType;
	CollType m_list;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_list.begin(); }
	const_iterator end() const { return m_list.end(); }
	iterator begin() { return m_list.begin(); }
	iterator end() { return m_list.end(); }
	void clear() { m_list.clear(); }

  PartialTranslOptColl();

	/** destructor, cleans out list */
	~PartialTranslOptColl()
	{
	}
	
	void Add(TranslationOption *partialTranslOpt);
	void Add(const PartialTranslOptColl &other);
	
	/** clear out the list */
	void DetachAll()
	{
		m_list.clear();
		//		TRACE_ERR( "clearing out list of " << m_list.size() << " partial translation options\n";
	}
};
