#include "alignmentStruct.h"

using namespace std;
namespace TERCpp
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