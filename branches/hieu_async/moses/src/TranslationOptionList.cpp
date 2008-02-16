// $Id: TranslationOptionList.cpp 142 2007-10-12 14:31:44Z hieu $
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

#include <algorithm>
#include "TranslationOptionList.h"
#include "TranslationOption.h"
#include "Util.h"

using namespace std;

TranslationOptionList::~TranslationOptionList()
{
	RemoveAllInColl(m_vecTransOpt);
}

// helper for sort
struct CompareTranslationOption
{
 	bool operator() (const TranslationOption *a, const TranslationOption *b)
  {
 		return a->GetFutureScore() > b->GetFutureScore();
 	}
};

void TranslationOptionList::Sort()
{
	std::sort(m_vecTransOpt.begin(), m_vecTransOpt.end(), CompareTranslationOption());
	/*
	for (size_t i = 0 ; i < m_vecTransOpt.size() ; i++)
	{
		TRACE_ERR(*m_vecTransOpt[i] << endl);
	}
	*/
}

std::ostream& operator<<(std::ostream& out, const TranslationOptionList& obj)
{
	TranslationOptionList::CollectionType::const_iterator iter;
	for (iter = obj.begin(); iter != obj.end(); ++iter)
	{
		out << **iter << endl;
	}
	return out;
}

