#include "documentStructure.h"

using namespace std;
namespace TERCpp
{
    string documentStructure::toString()
    {
        stringstream s;
// 	s << "nword : " << vectorToString(nwords)<<endl;
// 	s << "alignment" << vectorToString(alignment)<<endl;
// 	s << "afterShift" << vectorToString(alignment)<<endl;
        s << "Nothing to be printed" <<endl;
        return s.str();
    }

    string documentStructure::getDocId()
    {
        return docId;
    }

    vector< segmentStructure > documentStructure::getSegments()
    {
        return seg;
    }

    string documentStructure::getSysId()
    {
        return sysId;
    }

    void documentStructure::addSegments ( segmentStructure s )
    {
      seg.push_back(s);
    }
    void documentStructure::addSegments ( string id, string text )
    {
      segmentStructure tmp_seg(id,text);
      seg.push_back(tmp_seg);
    }
    segmentStructure* documentStructure::getLastSegments()
    {
      return & seg.at((int)seg.size()-1);
    }


// 	documentStructure::documentStructure()
// 	{
// // 		vector<string> ref;
// // 		vector<string> hyp;
// // 		vector<string> aftershift;
//
// 		//   documentStructure[] allshifts = null;
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
// 	documentStructure::documentStructure ()
// 	{
// 		start = 0;
// 		end = 0;
// 		moveto = 0;
// 		newloc = 0;
// 		cost=1.0;
// 	}
// 	documentStructure::documentStructure (int _start, int _end, int _moveto, int _newloc)
// 	{
// 		start = _start;
// 		end = _end;
// 		moveto = _moveto;
// 		newloc = _newloc;
// 		cost=1.0;
// 	}

// 	documentStructure::documentStructure (int _start, int _end, int _moveto, int _newloc, vector<string> _shifted)
// 	{
// 		start = _start;
// 		end = _end;
// 		moveto = _moveto;
// 		newloc = _newloc;
// 		shifted = _shifted;
// 		cost=1.0;
// 	}
// 	string documentStructure::vectorToString(vector<string> vec)
// 	{
// 		string retour("");
// 		for (vector<string>::iterator vecIter=vec.begin();vecIter!=vec.end(); vecIter++)
// 		{
// 			retour+=(*vecIter)+"\t";
// 		}
// 		return retour;
// 	}

// 	string documentStructure::toString()
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
// 	int documentStructure::distance()
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
// 	bool documentStructure::leftShift()
// 	{
// 		return (moveto < start);
// 	}
//
// 	int documentStructure::size()
// 	{
// 		return (end - start) + 1;
// 	}
// 	documentStructure documentStructure::operator=(documentStructure t)
// 	{
//
// 		return t;
// 	}


}
