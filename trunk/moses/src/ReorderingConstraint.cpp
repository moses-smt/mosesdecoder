// $Id: ReorderingConstraint.cpp 988 2006-11-21 19:35:37Z hieuhoang1972 $
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2008 University of Edinburgh

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

#include "ReorderingConstraint.h"
#include "InputType.h"
#include "Word.h"

namespace Moses
{

void ReorderingConstraint::SetWall( const InputType& sentence )
{
	for( size_t i=0; i<sentence.GetSize(); i++ )
	{
		const Word& word = sentence.GetWord( i );
		if (word[0]->GetString() == "," ||
		    word[0]->GetString() == "." ||
		    word[0]->GetString() == "!" ||
		    word[0]->GetString() == "?" ||
		    word[0]->GetString() == ":" ||
		    word[0]->GetString() == ";" ||
		    word[0]->GetString() == "\"")
		{
			// std::cerr << "SETTING reordering wall at position " << i << std::endl;
			SetValue( i, true );
		}
	}
}

bool ReorderingConstraint::ContainsWall( size_t start, size_t end ) const
{
	for( size_t i=start; i<=end; i++ )
	{
		if ( GetWall( i ) ) {
			// std::cerr << "HITTING reordering wall at position " << i << std::endl;
			return true;
		}
	}
	return false;
}

}
