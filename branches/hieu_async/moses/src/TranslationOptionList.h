// $Id: TranslationOptionList.h 142 2007-10-12 14:31:44Z hieu $
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

class TranslationOption;

class TranslationOptionList
{
protected:
	friend std::ostream& operator<<(std::ostream& out, const TranslationOptionList& obj);

	typedef std::vector<const TranslationOption*> CollectionType;
	CollectionType m_vecTransOpt;

public:
	typedef CollectionType::iterator iterator;
	typedef CollectionType::const_iterator const_iterator;
	const_iterator begin() const { return m_vecTransOpt.begin(); }
	const_iterator end() const { return m_vecTransOpt.end(); }
	iterator begin() { return m_vecTransOpt.begin(); }
	iterator end() { return m_vecTransOpt.end(); }

	~TranslationOptionList();
	
	size_t GetSize() const
	{ return m_vecTransOpt.size(); }
	void Resize(size_t newSize)
	{ m_vecTransOpt.resize(newSize); }
	void Add(const TranslationOption *transOpt)
	{
		m_vecTransOpt.push_back(transOpt);
	}

	const TranslationOption * const & operator[](size_t index) const {
		return m_vecTransOpt[index];
	}

	void Sort();
};


