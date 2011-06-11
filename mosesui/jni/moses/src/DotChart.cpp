// $Id: DotChart.cpp 3424 2010-09-10 15:19:25Z pjwilliams $
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

#include <algorithm>

using namespace std;

namespace Moses
{
ProcessedRuleColl::~ProcessedRuleColl()
{
    std::for_each(m_coll.begin(), m_coll.end(), RemoveAllInColl<CollType::value_type>);
}

std::ostream& operator<<(std::ostream &out, const ProcessedRule &rule)
{
	//const PhraseDictionaryNode &node = rule.GetLastNode();
	//out << node;
	
	return out;
}

std::ostream& operator<<(std::ostream &out, const ProcessedRuleList &coll)
{
	ProcessedRuleList::const_iterator iter;
	for (iter = coll.begin(); iter != coll.end(); ++iter)
	{
		const ProcessedRule &rule = **iter;
		out << rule << endl;
		
	}
	
	return out;
}

};
