// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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
#include "../../moses/src/WordsRange.h"
#include "ChartTranslationOption.h"

namespace MosesChart
{

class ChartTranslationOptionOrderer
{
public:
	bool operator()(const TranslationOption* transOptA, const TranslationOption* transOptB) const
	{
		/*
		if (transOptA->GetArity() != transOptB->GetArity())
		{
			return transOptA->GetArity() < transOptB->GetArity();
		}
		*/
		return transOptA->GetTotalScore() > transOptB->GetTotalScore();
	}
};

class TranslationOptionList
{
	friend std::ostream& operator<<(std::ostream&, const TranslationOptionList&);

protected:
	typedef std::vector<TranslationOption*> CollType;
	CollType m_coll;
	Moses::WordsRange m_range;

public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	TranslationOptionList(const Moses::WordsRange &range)
	: m_range(range)
	{}

	~TranslationOptionList();

	size_t GetSize() const
	{ return m_coll.size();	}
	const Moses::WordsRange &GetSourceRange() const
	{ return m_range;	}
	void Add(TranslationOption *transOpt);

	void Sort();

	void Reserve(CollType::size_type n) { m_coll.reserve(n); }
};

}
