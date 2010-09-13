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

#include <algorithm>
#include <iostream>
#include "ChartTranslationOptionList.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/StaticData.h"

using namespace Moses;
using namespace std;

namespace MosesChart
{

TranslationOptionList::~TranslationOptionList()
{
	RemoveAllInColl(m_coll);
}

void TranslationOptionList::Add(TranslationOption *transOpt)
{
	m_coll.push_back(transOpt);
}

void TranslationOptionList::Sort()
{
	// keep only those over best + threshold

	float scoreThreshold = -std::numeric_limits<float>::infinity();
	CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const TranslationOption *transOpt = *iter;
		float score = transOpt->GetTotalScore();
		scoreThreshold = (score > scoreThreshold) ? score : scoreThreshold;
	}

	scoreThreshold += StaticData::Instance().GetTranslationOptionThreshold();

	size_t ind = 0;
	while (ind < m_coll.size())
	{
		const TranslationOption *transOpt = m_coll[ind];
		if (transOpt->GetTotalScore() < scoreThreshold)
		{
			delete transOpt;
			m_coll.erase(m_coll.begin() + ind);
		}
		else
		{
			ind++;
		}
	}

	std::sort(m_coll.begin(), m_coll.end(), ChartTranslationOptionOrderer());
}

std::ostream& operator<<(std::ostream &out, const TranslationOptionList &list)
{
	TranslationOptionList::const_iterator iter;
	for (iter = list.begin(); iter != list.end(); ++iter)
	{
		out << **iter << endl;
	}

	return out;
}

}

