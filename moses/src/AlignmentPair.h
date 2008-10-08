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
#include <vector>
#include <iterator>
#include "TypeDef.h"
#include "Util.h"
#include "AlignmentPhrase.h"

namespace Moses
{

typedef std::back_insert_iterator<AlignmentPhrase::CollectionType> AlignmentPhraseInserter;

/** represent the alignment info between source and target phrase */
class AlignmentPair
{
	friend std::ostream& operator<<(std::ostream&, const AlignmentPair&);

protected:
	AlignmentPhrase m_sourceAlign, m_targetAlign;

public:
	// constructor
	AlignmentPair()
	{}
	// constructor, init source size. used in hypo
	AlignmentPair(size_t sourceSize)
		:m_sourceAlign(sourceSize)
	{}
	
	// constructor, by copy
	AlignmentPair(const AlignmentPair& a){
		m_sourceAlign=a.GetAlignmentPhrase(Input);
		m_targetAlign=a.GetAlignmentPhrase(Output);
	};

	// constructor, by copy
	AlignmentPair(const AlignmentPhrase& a, const AlignmentPhrase& b){
		SetAlignmentPhrase(a,b);
	};
	
	~AlignmentPair(){};
		
	/** get the back_insert_iterator to the source or target alignment vector so that
		*	they could be populated
		*/
	AlignmentPhraseInserter GetInserter(FactorDirection direction);
	
	const AlignmentPhrase &GetAlignmentPhrase(FactorDirection direction) const
	{
		return (direction == Input) ? m_sourceAlign : m_targetAlign;
	}
	
	AlignmentPhrase &GetAlignmentPhrase(FactorDirection direction)
	{
		return (direction == Input) ? m_sourceAlign : m_targetAlign;
	}
	
	void SetAlignmentPhrase(FactorDirection direction, const AlignmentPhrase& a) 
	{
		if (direction == Input) m_sourceAlign=a;
		else m_targetAlign=a;
	}
	
	void SetAlignmentPhrase(const AlignmentPhrase& a, const AlignmentPhrase& b) 
	{
		m_sourceAlign=a;
		m_targetAlign=b;
	}
	

	/** used by the unknown word handler.
		* Set alignment to 0
		*/
	void SetIdentityAlignment();

	//! call Merge() for source and and Add() target alignment phrase
	void Add(const AlignmentPair &newAlignment, const WordsRange &sourceRange, const WordsRange &targetRange);

	//! call Merge for both source and target alignment phrase
	void Merge(const AlignmentPair &newAlignment, const WordsRange &sourceRange, const WordsRange &targetRange);

	bool IsCompatible(const AlignmentPair &compare
									, size_t sourceStart
									, size_t targetStart) const;

	TO_STRING();
};

}

