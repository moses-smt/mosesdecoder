/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/
#include "alignmentStruct.h"

using namespace std;
namespace TERCPPNS_TERCpp
{
string alignmentStruct::toString()
{
  stringstream s;
// 	s << "nword : " << vectorToString(nwords)<<endl;
// 	s << "alignment" << vectorToString(alignment)<<endl;
// 	s << "afterShift" << vectorToString(alignment)<<endl;
  s << "Nothing to be printed" <<endl;
  return s.str();
}
void alignmentStruct::set(alignmentStruct l_alignmentStruct)
{
  nwords=l_alignmentStruct.nwords; // The words we shifted
  alignment=l_alignmentStruct.alignment ; // for pra_more output
  aftershift=l_alignmentStruct.aftershift; // for pra_more output
  cost=l_alignmentStruct.cost;
}



// 	alignmentStruct::alignmentStruct()
// 	{
// // 		vector<string> ref;
// // 		vector<string> hyp;
// // 		vector<string> aftershift;
//
// 		//   alignmentStruct[] allshifts = null;
//
// 		numEdits=0;
// 		numWords=0;
// 		bestRef="";
//
// 		numIns=0;
// 		numDel=0;
// 		numSub=0;
// 		numSft=0;
// 		numWsf=0;
// 	}
// 	alignmentStruct::alignmentStruct ()
// 	{
// 		start = 0;
// 		end = 0;
// 		moveto = 0;
// 		newloc = 0;
// 		cost=1.0;
// 	}
// 	alignmentStruct::alignmentStruct (int _start, int _end, int _moveto, int _newloc)
// 	{
// 		start = _start;
// 		end = _end;
// 		moveto = _moveto;
// 		newloc = _newloc;
// 		cost=1.0;
// 	}

// 	alignmentStruct::alignmentStruct (int _start, int _end, int _moveto, int _newloc, vector<string> _shifted)
// 	{
// 		start = _start;
// 		end = _end;
// 		moveto = _moveto;
// 		newloc = _newloc;
// 		shifted = _shifted;
// 		cost=1.0;
// 	}
// 	string alignmentStruct::vectorToString(vector<string> vec)
// 	{
// 		string retour("");
// 		for (vector<string>::iterator vecIter=vec.begin();vecIter!=vec.end(); vecIter++)
// 		{
// 			retour+=(*vecIter)+"\t";
// 		}
// 		return retour;
// 	}

// 	string alignmentStruct::toString()
// 	{
// 		stringstream s;
// 		s.str("");
// 		s << "[" << start << ", " << end << ", " << moveto << "/" << newloc << "]";
// 		if ((int)shifted.size() > 0)
// 		{
// 			s << " (" << vectorToString(shifted) << ")";
// 		}
// 		return s.str();
// 	}

/* The distance of the shift. */
// 	int alignmentStruct::distance()
// 	{
// 		if (moveto < start)
// 		{
// 			return start - moveto;
// 		}
// 		else if (moveto > end)
// 		{
// 			return moveto - end;
// 		}
// 		else
// 		{
// 			return moveto - start;
// 		}
// 	}
//
// 	bool alignmentStruct::leftShift()
// 	{
// 		return (moveto < start);
// 	}
//
// 	int alignmentStruct::size()
// 	{
// 		return (end - start) + 1;
// 	}
// 	alignmentStruct alignmentStruct::operator=(alignmentStruct t)
// 	{
//
// 		return t;
// 	}


}
