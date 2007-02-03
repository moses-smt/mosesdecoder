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

#include "Factor.h"

using namespace std;

size_t Factor::s_id = 0;

Factor::Factor(FactorDirection direction, FactorType factorType, const std::string *factorString)
:m_direction(direction)
,m_factorType(factorType)
,m_ptrString(factorString)
,m_id(NOT_FOUND)
{
}

TO_STRING_BODY(Factor)

// friend
ostream& operator<<(ostream& out, const Factor& factor)
{
	out << factor.GetString();
	return out;
}

