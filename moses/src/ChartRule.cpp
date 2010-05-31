// $Id: ChartRule.cpp 3048 2010-04-05 17:25:26Z hieuhoang1972 $
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

#include "ChartRule.h"
#include "TargetPhrase.h"
#include "AlignmentInfo.h"
#include "WordConsumed.h"

using namespace std;

namespace Moses
{

void ChartRule::CreateNonTermIndex()
{
	m_wordsConsumedTargetOrder.resize(m_targetPhrase.GetSize(), NOT_FOUND);
	const AlignmentInfo &alignInfo = m_targetPhrase.GetAlignmentInfo();

	size_t nonTermInd = 0;
	size_t prevSourcePos = 0;
	AlignmentInfo::const_iterator iter;
	for (iter = alignInfo.begin(); iter != alignInfo.end(); ++iter)
	{
		// alignment pairs must be ordered by source index
		size_t sourcePos = iter->first;
		if (nonTermInd > 0)
		{
			assert(sourcePos > prevSourcePos);
		}
		prevSourcePos = sourcePos;

		size_t targetPos = iter->second;
		m_wordsConsumedTargetOrder[targetPos] = nonTermInd;
		nonTermInd++;
	}
}

std::ostream& operator<<(std::ostream &out, const ChartRule &rule)
{
	out << rule.m_lastWordConsumed << ": " << rule.m_targetPhrase.GetTargetLHS() << "->" << rule.m_targetPhrase;
	return out;
}

} // namespace

