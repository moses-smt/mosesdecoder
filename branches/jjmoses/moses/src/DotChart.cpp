// $Id: DotChart.cpp 3048 2010-04-05 17:25:26Z hieuhoang1972 $
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
#include "DotChart.h"
#include "Util.h"
#include "PhraseDictionaryNodeNewFormat.h"

using namespace std;

namespace Moses
{
ProcessedRuleStack::ProcessedRuleStack(size_t size)
:m_coll(size)
{
	for (size_t ind = 0; ind < size; ++ind)
	{
		m_coll[ind] = new ProcessedRuleColl();
	}
}

ProcessedRuleStack::~ProcessedRuleStack()
{
	RemoveAllInColl(m_coll);
	RemoveAllInColl(m_savedNode);
}

std::ostream& operator<<(std::ostream &out, const ProcessedRule &rule)
{
	const PhraseDictionaryNode &node = rule.GetLastNode();
	//out << node;
	
	return out;
}

std::ostream& operator<<(std::ostream &out, const ProcessedRuleColl &coll)
{
	ProcessedRuleColl::CollType::const_iterator iter;
	for (iter = coll.begin(); iter != coll.end(); ++iter)
	{
		const ProcessedRule &rule = **iter;
		out << rule << endl;
		
	}
	
	return out;
}

};
